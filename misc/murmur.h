#ifndef _MURMUR_H_
#define _MURMUR_H_

#include <stdint.h>
#include <stddef.h>
#include "hdr.h"

struct murmurctx {
  uint32_t hash;
  uint32_t len;
};

#define MURMURCTX_INITER(seed) \
  { \
    .hash = (seed), \
    .len = 0, \
  }

static inline void murmurctx_feed32(struct murmurctx *ctx, uint32_t val)
{
  uint32_t c1 = 0xcc9e2d51;
  uint32_t c2 = 0x1b873593;
  uint32_t m = 5;
  uint32_t n = 0xe6546b64;
  uint32_t k = val;
  uint32_t hash = ctx->hash;
  k = k*c1;
  k = (k << 15) | (k >> 17);
  k = k*c2;
  hash = hash ^ k;
  hash = (hash << 13) | (hash >> 19);
  hash = hash*m + n;
  ctx->len += 4;
  ctx->hash = hash;
}

static inline void murmurctx_feed_buf(
  struct murmurctx *ctx, const void *buf, size_t sz)
{
  const char *cbuf = buf;
  size_t i = 0;
  struct murmurctx ctx2 = *ctx;
  while (i < (sz/4)*4)
  {
    murmurctx_feed32(&ctx2, hdr_get32h(&cbuf[i]));
    i += 4;
  }
  while (i < sz)
  {
    murmurctx_feed32(&ctx2, hdr_get8h(&cbuf[i]));
    i += 1;
  }
  *ctx = ctx2;
}

static inline uint32_t murmurctx_get(struct murmurctx *ctx)
{
  uint32_t hash;
  hash = ctx->hash;
  hash = hash ^ ctx->len;
  hash = hash ^ (hash >> 16);
  hash = hash * 0x85ebca6b;
  hash = hash ^ (hash >> 13);
  hash = hash * 0xc2b2ae35;
  hash = hash ^ (hash >> 16);
  return hash;
}


static inline uint32_t murmur32(uint32_t seed, uint32_t val)
{
  struct murmurctx ctx = MURMURCTX_INITER(seed);
  murmurctx_feed32(&ctx, val);
  return murmurctx_get(&ctx);
}

static inline uint32_t murmur_buf(uint32_t seed, const void *buf, size_t sz)
{
  struct murmurctx ctx = MURMURCTX_INITER(seed);
  murmurctx_feed_buf(&ctx, buf, sz);
  return murmurctx_get(&ctx);
}

#endif
