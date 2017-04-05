#ifndef _CHACHA_H_
#define _CHACHA_H_

#include <stdint.h>

struct chacha20_ctx {
  char key[32];
  char nonce[12];
  uint32_t counter;
};

static inline void chacha20_init(
  struct chacha20_ctx *ctx, char key[32], char nonce[12])
{
  memcpy(ctx->key, key, 32);
  memcpy(ctx->nonce, nonce, 12);
  ctx->counter = 0;
}

void chacha20_block(
  char key[32], uint32_t counter, char nonce[12], char out[64]);

static inline void chacha20_next_block(
  struct chacha20_ctx *ctx, char out[64])
{
  chacha20_block(ctx->key, ctx->counter++, ctx->nonce, out);
}

#endif
