#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "hdr.h"
#include "random_mt.h"

void random_mt_ctx_init(struct random_mt_ctx *ctx, uint32_t seed)
{
  int i;
  ctx->index = 624;
  ctx->mt[0] = seed;
  for (i = 1; i < 624; i++)
  {
    ctx->mt[i] = (1812433253 * (ctx->mt[i - 1] ^ (ctx->mt[i - 1] >> 30)) + i);
  }
}

void random_mt_twist(struct random_mt_ctx *ctx)
{
  int i;
  uint32_t y;
  static uint32_t mag01[2]={0x0U, 0x9908b0dfU};
  for (i = 0; i < 227; i++)
  {
    y = (ctx->mt[i] & 0x80000000) + (ctx->mt[(i + 1)] & 0x7fffffff);
    ctx->mt[i] = ctx->mt[i + 397] ^ (y >> 1) ^ mag01[y&1];
  }
  for (i = 227; i < 623; i++)
  {
    y = (ctx->mt[i] & 0x80000000) + (ctx->mt[(i + 1)] & 0x7fffffff);
    ctx->mt[i] = ctx->mt[(i + 397) - 624] ^ (y >> 1) ^ mag01[y&1];
  }
  for (i = 623; i < 624; i++)
  {
    y = (ctx->mt[i] & 0x80000000) + (ctx->mt[(i + 1) - 624] & 0x7fffffff);
    ctx->mt[i] = ctx->mt[(i + 397) - 624] ^ (y >> 1) ^ mag01[y&1];
  }
  ctx->index = 0;
}

void random_mt_bytes(struct random_mt_ctx *ctx, void *buf, size_t bufsiz)
{
  char *cbuf = buf;
  while (bufsiz >= 16)
  {
    hdr_set32h(&cbuf[0], random_mt(ctx));
    hdr_set32h(&cbuf[4], random_mt(ctx));
    hdr_set32h(&cbuf[8], random_mt(ctx));
    hdr_set32h(&cbuf[12], random_mt(ctx));
    cbuf += 16;
    bufsiz -= 16;
  }
  while (bufsiz >= 4)
  {
    hdr_set32h(&cbuf[0], random_mt(ctx));
    cbuf += 4;
    bufsiz -= 4;
  }
  while (bufsiz >= 1)
  {
    hdr_set8h(&cbuf[0], (uint8_t)random_mt(ctx));
    cbuf += 1;
    bufsiz -= 1;
  }
}
