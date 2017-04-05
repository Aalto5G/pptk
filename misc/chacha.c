#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "hdr.h"
#include "byteswap.h"
#include "chacha.h"

struct chacha20_internal_ctx {
  uint32_t state[16];
};

#define ROTL(x, i) \
  do { \
    x = (x<<(i)) | (x>>(32-(i))); \
  } \
  while(0)

static inline void quarterround(struct chacha20_internal_ctx *ctx, int ai, int bi, int ci, int di)
{
  uint32_t a = ctx->state[ai];
  uint32_t b = ctx->state[bi];
  uint32_t c = ctx->state[ci];
  uint32_t d = ctx->state[di];
  a += b; d ^= a; ROTL(d, 16);
  c += d; b ^= c; ROTL(b, 12);
  a += b; d ^= a; ROTL(d, 8);
  c += d; b ^= c; ROTL(b, 7);
  ctx->state[ai] = a;
  ctx->state[bi] = b;
  ctx->state[ci] = c;
  ctx->state[di] = d;
}

static inline void innerblock(struct chacha20_internal_ctx *ctx)
{
  quarterround(ctx, 0, 4, 8,12);
  quarterround(ctx, 1, 5, 9,13);
  quarterround(ctx, 2, 6,10,14);
  quarterround(ctx, 3, 7,11,15);
  quarterround(ctx, 0, 5,10,15);
  quarterround(ctx, 1, 6,11,12);
  quarterround(ctx, 2, 7, 8,13);
  quarterround(ctx, 3, 4, 9,14);
}

void chacha20_block(
  char key[32], uint32_t counter, char nonce[12], char out[64])
{
  struct chacha20_internal_ctx ctx, working_ctx;
  int i;
  ctx.state[0] = 0x61707865;
  ctx.state[1] = 0x3320646e;
  ctx.state[2] = 0x79622d32;
  ctx.state[3] = 0x6b206574;
  ctx.state[4] = byteswap32(hdr_get32n(&key[0]));
  ctx.state[5] = byteswap32(hdr_get32n(&key[4]));
  ctx.state[6] = byteswap32(hdr_get32n(&key[8]));
  ctx.state[7] = byteswap32(hdr_get32n(&key[12]));
  ctx.state[8] = byteswap32(hdr_get32n(&key[16]));
  ctx.state[9] = byteswap32(hdr_get32n(&key[20]));
  ctx.state[10] = byteswap32(hdr_get32n(&key[24]));
  ctx.state[11] = byteswap32(hdr_get32n(&key[28]));
  ctx.state[12] = counter;
  ctx.state[13] = byteswap32(hdr_get32n(&nonce[0]));
  ctx.state[14] = byteswap32(hdr_get32n(&nonce[4]));
  ctx.state[15] = byteswap32(hdr_get32n(&nonce[8]));
  working_ctx = ctx;
  for (i = 1; i <= 10; i++)
  {
    innerblock(&working_ctx);
  }
  for (i = 0; i < 16; i++)
  {
    ctx.state[i] += working_ctx.state[i];
  }
  for (i = 0; i < 16; i++)
  {
    hdr_set32n(&out[4*i], byteswap32(ctx.state[i]));
  }
}
