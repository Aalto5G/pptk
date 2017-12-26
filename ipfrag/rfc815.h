#ifndef _RFC815_H_
#define _RFC815_H_

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
  size_t hdr_len;
  enum packet_direction direction;
  char pkt_header[128];
  char pkt[65535];
};

struct packet *rfc815ctx_reassemble(struct as_alloc_local *loc, struct rfc815ctx *ctx);

void rfc815ctx_init(struct rfc815ctx *ctx);

void rfc815ctx_init_fast(struct rfc815ctx *ctx);

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

void rfc815ctx_add(struct rfc815ctx *ctx, struct packet *pkt);

#endif
