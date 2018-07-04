#ifndef _IPREASS_H_
#define _IPREASS_H_

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "allocif.h"
#include "packet.h"
#include "ipfrag.h"

struct rb_explicit_reassctx {
  struct linked_list_head packet_list;
  struct rb_tree hole_tree;
  struct rbhole first_hole;
  uint16_t most_restricting_last;
};

void rb_explicit_reassctx_init(struct rb_explicit_reassctx *ctx);

static inline int rb_explicit_reassctx_complete(struct rb_explicit_reassctx *ctx)
{
  return ctx->hole_tree.root == NULL;
}

void rb_explicit_reassctx_free(struct allocif *loc, struct rb_explicit_reassctx *ctx);

struct packet *
rb_explicit_reassctx_reassemble(struct allocif *loc, struct rb_explicit_reassctx *ctx);

void rb_explicit_reassctx_add(struct allocif *loc,
                              struct rb_explicit_reassctx *ctx, struct packet *pkt);

#endif
