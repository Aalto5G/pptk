#ifndef _RBCOMBO_H_
#define _RBCOMBO_H_

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "llalloc.h"
#include "iphdr.h"
#include "packet.h"
#include "ipcksum.h"
#include "ipfrag.h"
#include "containerof.h"
#include "iprb815.h"
#include "iprbexplicit.h"

struct rbcomboctx {
  int rfc_active;
  int packet_count;
  union {
    struct rb815ctx *rfc;
    struct rb_explicit_reassctx reass;
  } u;
};

void rbcomboctx_init(struct rbcomboctx *ctx);

static inline int rbcomboctx_complete(struct rbcomboctx *ctx)
{
  if (ctx->rfc_active)
  {
    return rb815ctx_complete(ctx->u.rfc);
  }
  return rb_explicit_reassctx_complete(&ctx->u.reass);
}

void rbcomboctx_free(struct allocif *loc, struct rbcomboctx *ctx);

void rbcomboctx_promote(struct allocif *loc, struct rbcomboctx *ctx);

void rbcomboctx_add(
  struct allocif *loc, struct rbcomboctx *ctx, struct packet *pkt);

struct packet *
rbcomboctx_reassemble(struct allocif *loc, struct rbcomboctx *ctx);

#endif
