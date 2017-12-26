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
#include "combo.h"

void comboctx_init(struct comboctx *ctx)
{
  ctx->rfc_active = 0;
  ctx->packet_count = 0;
  reassctx_init(&ctx->u.reass);
}

void comboctx_free(struct as_alloc_local *loc, struct comboctx *ctx)
{
  if (ctx->rfc_active)
  {
    free(ctx->u.rfc);
    ctx->u.rfc = NULL;
    return;
  }
  reassctx_free(loc, &ctx->u.reass);
}

void comboctx_promote(struct as_alloc_local *loc, struct comboctx *ctx)
{
  struct rfc815ctx *newctx;
  struct linked_list_node *iter;
  if (ctx->rfc_active)
  {
    return;
  }
  newctx = malloc(sizeof(*newctx));
  if (newctx == NULL)
  {
    return;
  }
  rfc815ctx_init(newctx);
  LINKED_LIST_FOR_EACH(iter, &ctx->u.reass.packet_list)
  {
    struct packet *pkt = CONTAINER_OF(iter, struct packet, node);
    rfc815ctx_add(newctx, pkt);
  }
  reassctx_free(loc, &ctx->u.reass);
  ctx->u.rfc = newctx;
  ctx->rfc_active = 1;
}

void comboctx_add(
  struct as_alloc_local *loc, struct comboctx *ctx, struct packet *pkt)
{
  if (!ctx->rfc_active && ctx->packet_count > 65535/1514)
  {
    comboctx_promote(loc, ctx);
  }
  ctx->packet_count++;
  if (ctx->rfc_active)
  {
    rfc815ctx_add(ctx->u.rfc, pkt);
    as_free_mt(loc, pkt);
    return;
  }
  reassctx_add(&ctx->u.reass, pkt);
}

struct packet *
comboctx_reassemble(struct as_alloc_local *loc, struct comboctx *ctx)
{
  if (ctx->rfc_active)
  {
    return rfc815ctx_reassemble(loc, ctx->u.rfc);
  }
  return reassctx_reassemble(loc, &ctx->u.reass);
}
