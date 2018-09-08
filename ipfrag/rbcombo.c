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
#include "rbcombo.h"

void rbcomboctx_init(struct rbcomboctx *ctx)
{
  ctx->rfc_active = 0;
  ctx->packet_count = 0;
  rb_explicit_reassctx_init(&ctx->u.reass, 0);
}

void rbcomboctx_free(struct allocif *loc, struct rbcomboctx *ctx)
{
  if (ctx->rfc_active)
  {
    free(ctx->u.rfc);
    ctx->u.rfc = NULL;
    return;
  }
  rb_explicit_reassctx_free(loc, &ctx->u.reass);
}

void rbcomboctx_promote(struct allocif *loc, struct rbcomboctx *ctx)
{
  struct rb815ctx *newctx;
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
  rb815ctx_init(newctx);
  LINKED_LIST_FOR_EACH(iter, &ctx->u.reass.packet_list)
  {
    struct packet *pkt = CONTAINER_OF(iter, struct packet, node);
    rb815ctx_add(newctx, pkt);
  }
  rb_explicit_reassctx_free(loc, &ctx->u.reass);
  ctx->u.rfc = newctx;
  ctx->rfc_active = 1;
}

void rbcomboctx_add(
  struct allocif *loc, struct rbcomboctx *ctx, struct packet *pkt)
{
  if (!ctx->rfc_active && ctx->packet_count > 65535/1514)
  {
    rbcomboctx_promote(loc, ctx);
  }
  ctx->packet_count++;
  if (ctx->rfc_active)
  {
    rb815ctx_add(ctx->u.rfc, pkt);
    allocif_free(loc, pkt);
    return;
  }
  rb_explicit_reassctx_add(loc, &ctx->u.reass, pkt, NULL);
}

struct packet *
rbcomboctx_reassemble(struct allocif *loc, struct rbcomboctx *ctx)
{
  if (ctx->rfc_active)
  {
    return rb815ctx_reassemble(loc, ctx->u.rfc);
  }
  return rb_explicit_reassctx_reassemble(loc, &ctx->u.reass);
}
