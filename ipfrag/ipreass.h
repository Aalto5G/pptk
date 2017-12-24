#ifndef _IPREASS_H_
#define _IPREASS_H_

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "asalloc.h"
#include "packet.h"
#include "ipfrag.h"

struct reassctx {
  uint32_t src_ip;
  uint32_t dst_ip;
  uint16_t ip_id;
  uint8_t proto;
  struct linked_list_head packet_list;
  struct linked_list_head hole_list;
  struct hole first_hole;
  uint16_t most_restricting_last;
};

void reassctx_init(struct reassctx *ctx);

static inline int reassctx_complete(struct reassctx *ctx)
{
  return linked_list_is_empty(&ctx->hole_list);
}

void reassctx_free(struct as_alloc_local *loc, struct reassctx *ctx);

struct packet *
reassctx_reassemble(struct as_alloc_local *loc, struct reassctx *ctx);

void reassctx_add(struct reassctx *ctx, struct packet *pkt);

#endif
