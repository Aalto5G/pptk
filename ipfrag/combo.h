#ifndef _COMBO_H_
#define _COMBO_H_

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
#include "rfc815.h"
#include "ipreass.h"

struct comboctx {
  int rfc_active;
  int packet_count;
  union {
    struct rfc815ctx *rfc;
    struct reassctx reass;
  } u;
};

void comboctx_init(struct comboctx *ctx);

static inline int comboctx_complete(struct comboctx *ctx)
{
  if (ctx->rfc_active)
  {
    return rfc815ctx_complete(ctx->u.rfc);
  }
  return reassctx_complete(&ctx->u.reass);
}

void comboctx_free(struct allocif *loc, struct comboctx *ctx);

void comboctx_promote(struct allocif *loc, struct comboctx *ctx);

void comboctx_add(
  struct allocif *loc, struct comboctx *ctx, struct packet *pkt);

struct packet *
comboctx_reassemble(struct allocif *loc, struct comboctx *ctx);

#endif
