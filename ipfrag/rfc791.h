#ifndef _RFC791_H_
#define _RFC791_H_

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

struct rfc791ctx {
  uint32_t src_ip;
  uint32_t dst_ip;
  uint16_t ip_id;
  uint8_t proto;
  uint16_t most_restricting_last;
  struct linked_list_head packet_list;
  uint64_t bits[128];
};

struct packet *rfc791ctx_reassemble(struct as_alloc_local *loc, struct rfc791ctx *ctx);

void rfc791ctx_init(struct rfc791ctx *ctx);

void rfc791ctx_free(struct as_alloc_local *loc, struct rfc791ctx *ctx);

int rfc791ctx_complete(struct rfc791ctx *ctx);

void rfc791ctx_add(struct rfc791ctx *ctx, struct packet *pkt);

#endif
