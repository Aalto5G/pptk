#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "asalloc.h"
#include "iphdr.h"
#include "packet.h"
#include "ipcksum.h"
#include "ipfrag.h"
#include "containerof.h"

struct rfc815hole {
  uint16_t first;
  uint16_t last;
  uint16_t next_hole;
  uint16_t prev_hole;
};

struct rfc815ctx {
  uint32_t src_ip;
  uint32_t dst_ip;
  uint16_t ip_id;
  uint8_t proto;
  uint16_t first_hole; // 65535 == initial link
  uint16_t last_hole; // 65535 == initial link
  uint16_t most_restricting_last;
  size_t hdr_len; // FIXME
  enum packet_direction direction;
  char pkt_header[128]; // FIXME
  char pkt[65535];
};

static struct packet *rfc815ctx_reassemble(struct as_alloc_local *loc, struct rfc815ctx *ctx)
{
  struct packet *pkt;
  size_t sz;
  char *ether2, *ip2, *pay2;
  sz = ctx->hdr_len + ctx->most_restricting_last + 1;
  pkt = as_alloc_mt(loc, packet_size(sz));
  if (pkt == NULL)
  {
    return NULL;
  }
  pkt->sz = sz;
  pkt->direction = ctx->direction;
  ether2 = packet_data(pkt);
  //printf("hdr_len %zu\n", (size_t)ctx->hdr_len);
  memcpy(ether2, ctx->pkt_header, ctx->hdr_len);
  ip2 = ether_payload(ether2);
  ip_set_frag_off(ip2, 0);
  ip_set_more_frags(ip2, 0);
  ip_set_total_len(ip2, ctx->hdr_len - 14 + ctx->most_restricting_last + 1);
  ip_set_hdr_cksum_calc(ip2, ctx->hdr_len - 14);
  pay2 = ether2 + ctx->hdr_len;
  memcpy(pay2, ctx->pkt, ctx->most_restricting_last + 1);
  return pkt;
}

static void rfc815ctx_init(struct rfc815ctx *ctx)
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
  memset(ctx->pkt, 0, sizeof(ctx->pkt));
  memcpy(&ctx->pkt[0], &hole, sizeof(hole));
}

static inline int rfc815ctx_complete(struct rfc815ctx *ctx)
{
  if (ctx->first_hole == 65535 && ctx->last_hole != 65535)
  {
    //printf("AB1\n");
    abort();
  }
  if (ctx->first_hole != 65535 && ctx->last_hole == 65535)
  {
    //printf("AB2\n");
    abort();
  }
  return ctx->first_hole == 65535;
}

#if 0
static struct rfc815hole holenonptr(struct rfc815ctx *ctx, uint16_t idx)
{
  struct rfc815hole hole;
  memcpy(&hole, ctx->pkt[idx], sizeof(hole));
  return hole;
}
#endif

static void holenonptr_set_prev(struct rfc815ctx *ctx, uint16_t idx, uint16_t prev)
{
  struct rfc815hole hole;
  if (idx == 65535)
  {
    ctx->last_hole = prev;
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
  }
  memcpy(&hole, &ctx->pkt[idx], sizeof(hole));
  hole.next_hole = next;
  memcpy(&ctx->pkt[idx], &hole, sizeof(hole));
}

#if 0
static uint16_t holenonptr_prev(struct rfc815ctx *ctx, uint16_t idx)
{
  struct rfc815hole hole;
  if (idx == 65535)
  {
    return ctx->last_hole;
  }
  memcpy(&hole, &ctx->pkt[idx], sizeof(hole));
  return hole.prev_hole;
}

static uint16_t holenonptr_next(struct rfc815ctx *ctx, uint16_t idx)
{
  struct rfc815hole hole;
  if (idx == 65535)
  {
    return ctx->first_hole;
  }
  memcpy(&hole, &ctx->pkt[idx], sizeof(hole));
  return hole.next_hole;
}
#endif

static void linktest(struct rfc815ctx *ctx)
{
  uint16_t iter;
  uint16_t prev = 65535;
  iter = ctx->first_hole;
  //printf("start iter %d\n", iter);
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
    //printf("iterating %d\n", iter);
  }
  if (ctx->last_hole != prev)
  {
    //printf("err3\n");
    abort();
  }
}

static void rfc815ctx_add(struct rfc815ctx *ctx, struct packet *pkt)
{
  const char *ether = packet_data(pkt);
  const char *ip = ether_const_payload(ether);
  uint16_t data_first;
  uint16_t data_last;
  uint16_t iter;
  linktest(ctx);
  if (ip_total_len(ip) <= ip_hdr_len(ip))
  {
    return;
  }
  if (ctx->hdr_len == 0)
  {
    ctx->hdr_len = 14 + ip_hdr_len(ip);
    memcpy(ctx->pkt_header, ether, ctx->hdr_len);
  }
  // FIXME verify there is total_len of data
  data_first = ip_frag_off(ip);
  // FIXME overflows:
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
        break;
      }
      else
      {
        hole.last = data_first - 1;
        memcpy(&ctx->pkt[iter], &hole, sizeof(hole));
        break;
      }
    }
    else if (data_last < hole.last)
    {
      if (data_first <= hole.first)
      {
        hole.first = data_last + 1;
        holenonptr_set_next(ctx, hole.prev_hole, data_last + 1);
        holenonptr_set_prev(ctx, hole.next_hole, data_last + 1);
        //printf("suspicious code path\n");
        memcpy(&ctx->pkt[data_last + 1], &hole, sizeof(hole));
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
        //printf("unimplemented code path %d %d\n", data_first, data_last);
        break;
        // FIXME add new hole
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
        //printf("continuing 1 %d\n", iter);
        continue;
      }
      else // data_last > hole.last, data_first > hole.first
      {
        if (hole.last > data_first - 1)
        {
          hole.last = data_first - 1;
          memcpy(&ctx->pkt[iter], &hole, sizeof(hole));
        }
        iter = hole.next_hole;
        //printf("continuing 2 %d\n", iter);
        continue;
      }
    }
  }
  memcpy(&ctx->pkt[data_first], ip_const_payload(ip), data_last - data_first + 1);
}




int main(int argc, char **argv)
{
  struct as_alloc_global glob;
  struct as_alloc_local loc;
  struct fragment fragment[2];
  char pkt[2102] = {0};
  size_t sz = sizeof(pkt);
  char *ether = pkt;
  char *ip;
  char *tcp;
  char edst[6] = {0x02,0x00,0x00,0x00,0x00,0x01};
  char esrc[6] = {0x02,0x00,0x00,0x00,0x00,0x02};
  struct packet *reassembled;
  struct rfc815ctx ctx;
  int i, j;

  memcpy(ether_dst(ether), edst, 6);
  memcpy(ether_src(ether), esrc, 6);
  ether_set_type(ether, ETHER_TYPE_IP);
  ip = ether_payload(ether);
  ip_set_version(ip, 4);
  ip_set_hdr_len(ip, 20);
  ip_set_total_len(ip, sizeof(pkt) - 14);
  ip_set_id(ip, 0x1234);
  ip_set_ttl(ip, 64);
  ip_set_proto(ip, 6);
  ip_set_src(ip, (10<<24) | 1);
  ip_set_dst(ip, (10<<24) | 2);
  ip_set_hdr_cksum_calc(ip, 20);
  tcp = ip_payload(ip);
  tcp_set_src_port(tcp, 12345);
  tcp_set_dst_port(tcp, 54321);
  tcp_set_ack_on(tcp);
  tcp_set_seq_number(tcp, 0x12345678);
  tcp_set_ack_number(tcp, 0x87654321);
  tcp_set_window(tcp, 32768);
  tcp_set_data_offset(tcp, 20);
  memset(((char*)tcp) + 20, 'X', sizeof(pkt)-14-20-20);
  tcp_set_cksum_calc(ip, 20, tcp, sizeof(pkt)-14-20);
  as_alloc_global_init(&glob, 1000, 1536);
  as_alloc_local_init(&loc, &glob, 1000);
  fragment[0].datastart = 0;
  fragment[0].datalen = 1514 - 14 - 20;
  fragment[0].pkt = NULL;
  fragment[1].datastart = 1514 - 14 - 20;
  fragment[1].datalen = sz - 14 - 20 - (1514 - 14 - 20);
  fragment[1].pkt = NULL;
  if (fragment4(&loc, pkt, sz, fragment, 2) != 0)
  {
    abort();
  }

  rfc815ctx_init(&ctx);
  rfc815ctx_add(&ctx, fragment[0].pkt);
  printf("beginning test 1\n");
  if (rfc815ctx_complete(&ctx))
  {
    printf("1\n");
    abort();
  }
  printf("beginning test 1\n");
  rfc815ctx_add(&ctx, fragment[1].pkt);
  printf("beginning test 1\n");
  if (!rfc815ctx_complete(&ctx))
  {
    printf("2\n");
    abort();
  }
  printf("beginning test 1\n");
  reassembled = rfc815ctx_reassemble(&loc, &ctx);
  if (reassembled->sz != sz)
  {
    printf("3\n");
    abort();
  }
#if 0
  if (memcmp(packet_data(reassembled), pkt, sz) != 0)
  {
    size_t si;
    for (si = 0; si < sz; si++)
    {
      if (((char*)packet_data(reassembled))[si] != (char)pkt[si])
      {
        printf("mismatch %zu\n", si);
        break;
      }
    }
    printf("4\n");
    abort();
  }
#endif
  as_free_mt(&loc, reassembled);

  printf("beginning test 2\n");

  rfc815ctx_init(&ctx);
  rfc815ctx_add(&ctx, fragment[1].pkt);
  if (rfc815ctx_complete(&ctx))
  {
    printf("5\n");
    abort();
  }
  rfc815ctx_add(&ctx, fragment[0].pkt);
  if (!rfc815ctx_complete(&ctx))
  {
    printf("6\n");
    abort();
  }
  reassembled = rfc815ctx_reassemble(&loc, &ctx);
  if (reassembled->sz != sz)
  {
    printf("7\n");
    abort();
  }
#if 0
  if (memcmp(packet_data(reassembled), pkt, sz) != 0)
  {
    printf("8\n");
    abort();
  }
#endif
  as_free_mt(&loc, reassembled);
  
  as_free_mt(&loc, fragment[0].pkt);
  as_free_mt(&loc, fragment[1].pkt);

  printf("beginning randomized tests\n");

  for (j = 0; j < 10*1000; j++)
  {
    rfc815ctx_init(&ctx);
    i = 0;
    for (;;)
    {
      fragment[0].datastart = ((rand() % (sz - 14 - 20)) >> 3) << 3;
      if (rand() % 2)
      {
        fragment[0].datalen = 0;
      }
      else
      {
        fragment[0].datalen = 1 + (rand() % (sz - 14 - 20 - fragment[0].datastart));
      }
      fragment[0].pkt = NULL;
      if (fragment4(&loc, pkt, sz, fragment, 1) != 0)
      {
        abort();
      }
      rfc815ctx_add(&ctx, fragment[0].pkt);
      as_free_mt(&loc, fragment[0].pkt);
      if (rfc815ctx_complete(&ctx))
      {
        break;
      }
      i++;
    }
    printf("%d packets\n", i);
    reassembled = rfc815ctx_reassemble(&loc, &ctx);
    if (reassembled->sz != sz)
    {
      printf("size mismatch %zu %zu\n", reassembled->sz, sz);
      abort();
    }
#if 0
    if (memcmp(packet_data(reassembled), pkt, sz) != 0)
    {
      printf("packet data mismatch\n");
      abort();
    }
#endif
    as_free_mt(&loc, reassembled);
  }

  as_alloc_local_free(&loc);
  as_alloc_global_free(&glob);
  
  return 0;
}
