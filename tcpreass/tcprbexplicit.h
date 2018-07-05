#ifndef _IPREASS_H_
#define _IPREASS_H_

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "allocif.h"
#include "packet.h"

/*
 * NB: this module misuses the "struct rbhole" in packet.h to mean
 * useful data, not a hole!
 */

struct tcp_rb_explicit_reassctx {
  uint32_t last_fed_seq_plus_1;
  struct rb_tree packet_tree;
};

void tcp_rb_explicit_reassctx_init(struct tcp_rb_explicit_reassctx *ctx,
                                   uint32_t isn);

void tcp_rb_explicit_reassctx_free(struct allocif *loc, struct tcp_rb_explicit_reassctx *ctx);

struct packet *
tcp_rb_explicit_reassctx_reassemble(struct allocif *loc, struct tcp_rb_explicit_reassctx *ctx);

struct packet *
tcp_rb_explicit_reassctx_add(struct allocif *loc,
                             struct tcp_rb_explicit_reassctx *ctx,
                             struct packet *pkt);

struct packet *
tcp_rb_explicit_reassctx_fetch(struct tcp_rb_explicit_reassctx *ctx);

#endif
