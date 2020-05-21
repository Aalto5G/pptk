#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "llalloc.h"
#include "linkedlist.h"
#include "iphdr.h"
#include "packet.h"
#include "ipcksum.h"
#include "ipfrag.h"
#include "containerof.h"
#include "rfc791.h"

struct packet *
rfc791ctx_reassemble(struct allocif *loc, struct rfc791ctx *ctx)
{
  struct packet *first_packet;
  const char *ether, *ip;
  char *ether2, *ip2, *pay2;
  size_t sz;
  uint16_t hdr_len;
  struct packet *pkt;
  struct linked_list_node *iter;
  if (linked_list_is_empty(&ctx->packet_list))
  {
    abort();
  }
  first_packet = CONTAINER_OF(ctx->packet_list.node.next, struct packet, node);
  ether = first_packet->data;
  ip = ether_const_payload(ether);
  hdr_len = ip_hdr_len(ip);
  sz = ETHER_HDR_LEN + hdr_len + ctx->most_restricting_last + 1;
  pkt = allocif_alloc(loc, packet_size(sz));
  if (pkt == NULL)
  {
    return NULL;
  }
  pkt->sz = sz;
  pkt->direction = first_packet->direction;
  pkt->data = packet_calc_data(pkt);
  ether2 = pkt->data;
  memcpy(ether2, ether, ETHER_HDR_LEN + hdr_len);
  ip2 = ether_payload(ether2);
  ip_set_frag_off(ip2, 0);
  ip_set_more_frags(ip2, 0);
  ip_set_total_len(ip2, hdr_len + ctx->most_restricting_last + 1);
  ip_set_hdr_cksum_calc(ip2, hdr_len);
  pay2 = ip_payload(ip2);
  LINKED_LIST_FOR_EACH(iter, &ctx->packet_list)
  {
    struct packet *pktorig = CONTAINER_OF(iter, struct packet, node);
    const char *etherorig, *iporig, *payorig;
    uint16_t first, len;
    etherorig = pktorig->data;
    iporig = ether_const_payload(etherorig);
    payorig = ip_const_payload(iporig);
    first = ip_frag_off(iporig);
    len = ip_total_len(iporig) - ip_hdr_len(iporig);
    if (first + len - 1 > ctx->most_restricting_last)
    {
      memcpy(pay2 + first, payorig, 1U + ctx->most_restricting_last - first);
    }
    else
    {
      memcpy(pay2 + first, payorig, len);
    }
  }
  return pkt;
}

void rfc791ctx_init(struct rfc791ctx *ctx)
{
  memset(ctx->bits, 0, sizeof(ctx->bits));
  ctx->most_restricting_last = 65535;
  linked_list_head_init(&ctx->packet_list);
}

void rfc791ctx_free(struct allocif *loc, struct rfc791ctx *ctx)
{
  struct linked_list_node *iter, *tmp;
  LINKED_LIST_FOR_EACH_SAFE(iter, tmp, &ctx->packet_list)
  {
    struct packet *pkt = CONTAINER_OF(iter, struct packet, node);
    linked_list_delete(&pkt->node);
    allocif_free(loc, pkt);
  }
}

void rfc791ctx_add(struct rfc791ctx *ctx, struct packet *pkt)
{
  const char *ether = pkt->data;
  const char *ip = ether_const_payload(ether);
  uint16_t data_first;
  uint16_t data_last;
  size_t firstmaxbit;
  size_t firsttotal;
  size_t onepastlasttotal;
  size_t secondminbit;
  size_t i;
  linked_list_add_tail(&pkt->node, &ctx->packet_list);
  //linktest(ctx);
  if (pkt->sz < 34 ||
      ip_total_len(ip) <= ip_hdr_len(ip) ||
      (size_t)(ip_total_len(ip) + 14) > pkt->sz)
  {
    return;
  }
  data_first = ip_frag_off(ip);
  if (ip_total_len(ip) - (ip_hdr_len(ip) + 1) + ip_frag_off(ip) > 65535)
  {
    return;
  }
  data_last = ip_total_len(ip) - (ip_hdr_len(ip) + 1) + ip_frag_off(ip);
  if (!ip_more_frags(ip) && ctx->most_restricting_last > data_last)
  {
    ctx->most_restricting_last = data_last;
  }
  firsttotal = (data_first + 8U*64U - 1U) / (8U*64U);
  onepastlasttotal = (data_last + 1U) / (8U*64U);
  firstmaxbit = firsttotal*64;
  secondminbit = onepastlasttotal*64U;
  if (firstmaxbit > (uint32_t)((data_last + 1U) / 8U))
  {
    firstmaxbit = (data_last + 1U) / 8U;
  }
  if (secondminbit < data_first / 8U)
  {
    secondminbit = data_first / 8U;
  }
#if 0
  for (i = data_first/8; i < (uint32_t)((data_last + 1) / 8); i++)
  {
    //printf("setting bit %zu\n", i);
    ctx->bits[i/64] |= 1ULL<<(i%64);
  }
#else
  //printf("%d-%d\n", data_first, data_last);
  for (i = data_first/8; i < firstmaxbit; i++)
  {
    //printf("setting bit %zu\n", i);
    ctx->bits[i/64] |= 1ULL<<(i%64);
  }
  for (i = firsttotal; i < onepastlasttotal; i++)
  {
    //printf("setting word %zu\n", i);
    ctx->bits[i] = UINT64_MAX;
  }
  for (i = secondminbit; i < (uint32_t)((data_last + 1) / 8); i++)
  {
    //printf("setting bit %zu\n", i);
    ctx->bits[i/64] |= 1ULL<<(i%64);
  }
#endif
}

int rfc791ctx_complete(struct rfc791ctx *ctx)
{ 
  size_t last_bit = ctx->most_restricting_last/8;
  size_t bitidx = last_bit / 64;
  size_t bitoff = last_bit % 64;
  size_t i;
  if (ctx->most_restricting_last == 65535)
  {
    return 0;
  }
  for (i = 0; i < bitidx; i++)
  {
    if (ctx->bits[i] != UINT64_MAX)
    {
      //printf("%zu: %llx\n", i, (unsigned long long)ctx->bits[i]);
      //printf("returning 0\n");
      return 0;
    }
  }
  //printf("calling ffsll\n");
  return ffsll((int64_t)~ctx->bits[bitidx]) == (int)(bitoff + 1);
}
