#ifndef _SIPHASH_H_
#define _SIPHASH_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include "hdr.h"

#define cROUNDS 2
#define dROUNDS 4

#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

struct siphash_ctx {
  uint64_t v0;
  uint64_t v1;
  uint64_t v2;
  uint64_t v3;
  uint64_t inlen;
  uint64_t b;
};

static inline void siphash_init(struct siphash_ctx *ctx, const void *k)
{
  const uint8_t *ku8 = k;
  uint64_t k0 = hdr_get64h(ku8);
  uint64_t k1 = hdr_get64h(ku8 + 8);
  ctx->v0 = 0x736f6d6570736575ULL;
  ctx->v1 = 0x646f72616e646f6dULL;
  ctx->v2 = 0x6c7967656e657261ULL;
  ctx->v3 = 0x7465646279746573ULL;
  ctx->v3 ^= k1;
  ctx->v2 ^= k0;
  ctx->v1 ^= k1;
  ctx->v0 ^= k0;
  ctx->inlen = 0;
  ctx->b = 0;
}

static inline void siphash_feed_u64(struct siphash_ctx *ctx, uint64_t in)
{
  int i;
  uint64_t m = in;
  ctx->v3 ^= m;

  for (i = 0; i < cROUNDS; ++i)
  {
    ctx->v0 += ctx->v1;
    ctx->v1 = ROTL(ctx->v1, 13);
    ctx->v1 ^= ctx->v0;
    ctx->v0 = ROTL(ctx->v0, 32);
    ctx->v2 += ctx->v3;
    ctx->v3 = ROTL(ctx->v3, 16);
    ctx->v3 ^= ctx->v2;
    ctx->v0 += ctx->v3;
    ctx->v3 = ROTL(ctx->v3, 21);
    ctx->v3 ^= ctx->v0;
    ctx->v2 += ctx->v1;
    ctx->v1 = ROTL(ctx->v1, 17);
    ctx->v1 ^= ctx->v2;
    ctx->v2 = ROTL(ctx->v2, 32);
  }

  ctx->v0 ^= m;
  ctx->inlen += 8;
}

static inline uint64_t siphash_get(struct siphash_ctx *ctx)
{
  uint64_t b;
  int i;

  ctx->b |= ((uint64_t)ctx->inlen) << 56;
  
  ctx->v3 ^= ctx->b;

  for (i = 0; i < cROUNDS; ++i)
  {
    ctx->v0 += ctx->v1;
    ctx->v1 = ROTL(ctx->v1, 13);
    ctx->v1 ^= ctx->v0;
    ctx->v0 = ROTL(ctx->v0, 32);
    ctx->v2 += ctx->v3;
    ctx->v3 = ROTL(ctx->v3, 16);
    ctx->v3 ^= ctx->v2;
    ctx->v0 += ctx->v3;
    ctx->v3 = ROTL(ctx->v3, 21);
    ctx->v3 ^= ctx->v0;
    ctx->v2 += ctx->v1;
    ctx->v1 = ROTL(ctx->v1, 17);
    ctx->v1 ^= ctx->v2;
    ctx->v2 = ROTL(ctx->v2, 32);
  }

  ctx->v0 ^= ctx->b;

  ctx->v2 ^= 0xff;

  for (i = 0; i < dROUNDS; ++i)
  {
    ctx->v0 += ctx->v1;
    ctx->v1 = ROTL(ctx->v1, 13);
    ctx->v1 ^= ctx->v0;
    ctx->v0 = ROTL(ctx->v0, 32);
    ctx->v2 += ctx->v3;
    ctx->v3 = ROTL(ctx->v3, 16);
    ctx->v3 ^= ctx->v2;
    ctx->v0 += ctx->v3;
    ctx->v3 = ROTL(ctx->v3, 21);
    ctx->v3 ^= ctx->v0;
    ctx->v2 += ctx->v1;
    ctx->v1 = ROTL(ctx->v1, 17);
    ctx->v1 ^= ctx->v2;
    ctx->v2 = ROTL(ctx->v2, 32);
  }

  b = ctx->v0 ^ ctx->v1 ^ ctx->v2 ^ ctx->v3;
  return b;
}

static inline void siphash_feed_remaining(
  struct siphash_ctx *ctx, const void *buf, size_t remaining)
{
  uint64_t b = 0;
  const unsigned char *in = buf;
  if (remaining >= 8)
  {
    abort();
  }
  if (ctx->b != 0)
  {
    abort();
  }
  switch (remaining) {
  case 7:
      b |= ((uint64_t)in[6]) << 48;
  case 6:
      b |= ((uint64_t)in[5]) << 40;
  case 5:
      b |= ((uint64_t)in[4]) << 32;
  case 4:
      b |= ((uint64_t)in[3]) << 24;
  case 3:
      b |= ((uint64_t)in[2]) << 16;
  case 2:
      b |= ((uint64_t)in[1]) << 8;
  case 1:
      b |= ((uint64_t)in[0]);
      break;
  case 0:
      break;
  }
  ctx->b = b;
  ctx->inlen += remaining;
}

// Slightly breaks the spec if buflen not multiple of 8
static inline void siphash_feed_buf(
  struct siphash_ctx *ctx, const void *buf, size_t buflen)
{
  const unsigned char *cbuf = buf;
  uint64_t b = 0;
  while (buflen >= 8)
  {
    siphash_feed_u64(ctx, hdr_get32h(cbuf));
    cbuf += 8;
    buflen -= 8;
  }
  switch (buflen) {
  case 7:
      b |= ((uint64_t)cbuf[6]) << 48;
  case 6:
      b |= ((uint64_t)cbuf[5]) << 40;
  case 5:
      b |= ((uint64_t)cbuf[4]) << 32;
  case 4:
      b |= ((uint64_t)cbuf[3]) << 24;
  case 3:
      b |= ((uint64_t)cbuf[2]) << 16;
  case 2:
      b |= ((uint64_t)cbuf[1]) << 8;
  case 1:
      b |= ((uint64_t)cbuf[0]);
      break;
  case 0:
      break;
  }
  siphash_feed_u64(ctx, b);
}

// This version does not break the spec but cannot be used incrementally
static inline uint64_t siphash_buf(
  const void *key, const void *buf, size_t buflen)
{
  struct siphash_ctx ctx;
  const char *cbuf = buf;
  siphash_init(&ctx, key);
  while (buflen >= 8)
  {
    siphash_feed_u64(&ctx, hdr_get32h(cbuf));
    cbuf += 8;
    buflen -= 8;
  }
  siphash_feed_remaining(&ctx, cbuf, buflen);
  return siphash_get(&ctx);
}

#endif
