#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "allocif.h"
#include "iphdr.h"
#include "packet.h"
#include "ipcksum.h"
#include "ipfrag.h"
#include "containerof.h"
#include "rfc815.h"

struct packet *rfc815ctx_reassemble(struct allocif *loc, struct rfc815ctx *ctx)
{
  struct packet *pkt;
  size_t sz;
  char *ether2, *ip2, *pay2;
  sz = ctx->hdr_len + ctx->most_restricting_last + 1;
  pkt = allocif_alloc(loc, packet_size(sz));
  if (pkt == NULL)
  {
    return NULL;
  }
  pkt->sz = sz;
  pkt->direction = ctx->direction;
  pkt->data = packet_calc_data(pkt);
  ether2 = pkt->data;
  //printf("hdr_len %zu\n", (size_t)ctx->hdr_len);
  memcpy(ether2, ctx->pkt_header, ctx->hdr_len);
  ip2 = ether_payload(ether2);
  ip_set_frag_off(ip2, 0);
  ip_set_more_frags(ip2, 0);
  ip_set_total_len(ip2, ctx->hdr_len - 14 + ctx->most_restricting_last + 1);
  ip_set_hdr_cksum_calc(ip2, ctx->hdr_len - 14);
  pay2 = ether2 + ctx->hdr_len;
  memcpy(pay2, ctx->pkt, ((size_t)ctx->most_restricting_last) + 1);
  return pkt;
}

void rfc815ctx_init(struct rfc815ctx *ctx)
{
  memset(ctx->pkt, 0, sizeof(ctx->pkt));
  rfc815ctx_init_fast(ctx);
}

void rfc815ctx_init_fast(struct rfc815ctx *ctx)
{
  struct rfc815hole hole;
  hole.first = 0;
  hole.last = 65535;
  hole.next_hole = 65535;
  hole.prev_hole = 65535;
  ctx->first_hole = 0;
  ctx->last_hole = 0;
  ctx->most_restricting_last = 65535;
  ctx->hdr_len = 0;
  memcpy(&ctx->pkt[0], &hole, sizeof(hole));
}

static void holenonptr_set_prev(struct rfc815ctx *ctx, uint16_t idx, uint16_t prev)
{
  struct rfc815hole hole;
  if (idx == 65535)
  {
    ctx->last_hole = prev;
    return;
  }
  memcpy(&hole, &ctx->pkt[idx], sizeof(hole));
  hole.prev_hole = prev;
  memcpy(&ctx->pkt[idx], &hole, sizeof(hole));
}

static void holenonptr_set_next(struct rfc815ctx *ctx, uint16_t idx, uint16_t next)
{
  struct rfc815hole hole;
  if (idx == 65535)
  {
    ctx->first_hole = next;
    return;
  }
  memcpy(&hole, &ctx->pkt[idx], sizeof(hole));
  hole.next_hole = next;
  memcpy(&ctx->pkt[idx], &hole, sizeof(hole));
}

#if 0
static void linktest(struct rfc815ctx *ctx)
{
  uint16_t iter;
  uint16_t prev = 65535;
  iter = ctx->first_hole;
  //printf("start iter\n");
  while (iter != 65535)
  {
    struct rfc815hole hole;
    memcpy(&hole, &ctx->pkt[iter], sizeof(hole));
    if (hole.first != iter)
    {
      //printf("err1\n");
      abort();
    }
    if (hole.prev_hole != prev)
    {
      //printf("err2\n");
      abort();
    }
    //printf("hole %d-%d\n", hole.first, hole.last);
    prev = iter;
    iter = hole.next_hole;
    if (iter < prev)
    {
      printf("list not well-sorted\n");
      abort();
    }
    //printf("iterating %d\n", iter);
  }
  if (ctx->last_hole != prev)
  {
    //printf("err3\n");
    abort();
  }
}
#endif

void rfc815ctx_add(struct rfc815ctx *ctx, struct packet *pkt)
{
  const char *ether = pkt->data;
  const char *ip = ether_const_payload(ether);
  uint16_t data_first;
  uint16_t data_last;
  uint16_t iter;
  int mod = 0;
  //linktest(ctx);
  if (pkt->sz < 34 ||
      ip_total_len(ip) <= ip_hdr_len(ip) ||
      (size_t)(ip_total_len(ip) + 14) > pkt->sz)
  {
    return;
  }
  if (ctx->hdr_len == 0)
  {
    ctx->hdr_len = 14U + ip_hdr_len(ip);
    memcpy(ctx->pkt_header, ether, ctx->hdr_len);
  }
  data_first = ip_frag_off(ip);
  if (ip_total_len(ip) - (ip_hdr_len(ip) + 1) + ip_frag_off(ip) > 65535)
  {
    return;
  }
  data_last = ip_total_len(ip) - (ip_hdr_len(ip) + 1) + ip_frag_off(ip);
  if (!ip_more_frags(ip) && ctx->most_restricting_last > data_last)
  {
    //printf("NOT MORE FRAGS, MOST RESTRICTING LAST %d, data_last %d\n",
    //       ctx->most_restricting_last, data_last);
    iter = ctx->first_hole;
    ctx->most_restricting_last = data_last;
    while (iter != 65535)
    {
      struct rfc815hole hole;
      memcpy(&hole, &ctx->pkt[iter], sizeof(hole));
      if (hole.first > ctx->most_restricting_last)
      {
        iter = hole.next_hole;
        //printf("DELETE1 %d\n", iter);
        holenonptr_set_next(ctx, hole.prev_hole, hole.next_hole);
        holenonptr_set_prev(ctx, hole.next_hole, hole.prev_hole);
        //printf("continue iter1 %d\n", iter);
        continue;
      }
      if (hole.last > ctx->most_restricting_last)
      {
        hole.last = ctx->most_restricting_last;
        memcpy(&ctx->pkt[iter], &hole, sizeof(hole));
        iter = hole.next_hole;
        //printf("continue iter2 %d\n", iter);
        continue;
      }
      iter = hole.next_hole;
    }
  }
  else
  {
    if (data_last < 7)
    {
      return;
    }
    data_last = (data_last + 1) / 8 * 8 - 1;
  }
  //printf("first %d last %d\n", data_first, data_last);
  iter = ctx->first_hole;
  while (iter != 65535)
  {
    struct rfc815hole hole;
    //printf("ItEr %d\n", iter);
    memcpy(&hole, &ctx->pkt[iter], sizeof(hole));
    if (data_last == hole.last)
    {
      if (data_first <= hole.first)
      {
        //printf("DELETE2 %d\n", iter);
        holenonptr_set_next(ctx, hole.prev_hole, hole.next_hole);
        holenonptr_set_prev(ctx, hole.next_hole, hole.prev_hole);
        mod = 1;
        break;
      }
      else
      {
        hole.last = data_first - 1;
        memcpy(&ctx->pkt[iter], &hole, sizeof(hole));
        mod = 1;
        break;
      }
    }
    else if (data_last < hole.last)
    {
      if (data_first <= hole.first)
      {
        if (data_last + 1 > hole.first)
        {
          hole.first = data_last + 1;
          holenonptr_set_next(ctx, hole.prev_hole, data_last + 1);
          holenonptr_set_prev(ctx, hole.next_hole, data_last + 1);
          memcpy(&ctx->pkt[data_last + 1], &hole, sizeof(hole));
          mod = 1;
        }
        break;
      }
      else
      {
        uint16_t old_next;
        uint16_t old_last;
        old_last = hole.last;
        old_next = hole.next_hole;
        hole.last = data_first - 1;
        holenonptr_set_prev(ctx, hole.next_hole, data_last + 1);
        hole.next_hole = data_last + 1;
        memcpy(&ctx->pkt[iter], &hole, sizeof(hole));
        hole.prev_hole = iter;
        hole.next_hole = old_next;
        hole.first = data_last + 1;
        hole.last = old_last;
        memcpy(&ctx->pkt[data_last + 1], &hole, sizeof(hole));
        mod = 1;
        break;
      }
    }
    else
    {
      // data_last > hole.last, data_first <= hole.first
      if (data_first <= hole.first)
      {
        //printf("DELETE3 %d\n", iter);
        iter = hole.next_hole;
        holenonptr_set_next(ctx, hole.prev_hole, hole.next_hole);
        holenonptr_set_prev(ctx, hole.next_hole, hole.prev_hole);
        mod = 1;
        //printf("continuing 1 %d\n", iter);
        continue;
      }
      else // data_last > hole.last, data_first > hole.first
      {
        if (hole.last > data_first - 1)
        {
          hole.last = data_first - 1;
          memcpy(&ctx->pkt[iter], &hole, sizeof(hole));
          mod = 1;
        }
        iter = hole.next_hole;
        //printf("continuing 2 %d\n", iter);
        continue;
      }
    }
  }
  if (mod)
  {
    memcpy(&ctx->pkt[data_first], ip_const_payload(ip), ((size_t)data_last) - ((size_t)data_first) + 1);
  }
}
