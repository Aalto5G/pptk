#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "byteswap.h"
#include "hdr.h"
#include "mypcap.h"
#include "time64.h"

void pcap_in_ctx_free(struct pcap_in_ctx *ctx)
{
  fclose(ctx->f);
  ctx->f = NULL;
}

void pcap_out_ctx_free(struct pcap_out_ctx *ctx)
{
  fclose(ctx->f);
  ctx->f = NULL;
}

int pcap_in_ctx_read(
  struct pcap_in_ctx *ctx, void **buf, size_t *bufcapacity,
  size_t *len, size_t *snap, uint64_t *time64)
{
  char hdr[16];
  uint32_t sec, usec;
  uint32_t incl_len;
  if (fread(hdr, 16, 1, ctx->f) != 1)
  {
    if (feof(ctx->f))
    {
      return 0;
    }
    return -EINVAL;
  }
  sec = hdr_get32h(&hdr[0]);
  if (ctx->swap)
  {
    sec = byteswap32(sec);
  }
  sec = (uint32_t)((int)sec + ctx->thiszone);
  usec = hdr_get32h(&hdr[4]);
  if (ctx->swap)
  {
    usec = byteswap32(usec);
  }
  if (ctx->ns)
  {
    usec += 500;
    usec /= 1000;
  }
  incl_len = hdr_get32h(&hdr[8]);
  if (ctx->swap)
  {
    incl_len = byteswap32(incl_len);
  }
  if (snap)
  {
    *snap = incl_len;
  }
  if (len)
  {
    *len = hdr_get32h(&hdr[12]);
    if (ctx->swap)
    {
      *len = byteswap32(*len);
    }
  }
  if (incl_len > *bufcapacity || *buf == NULL)
  {
    size_t newcapacity = 2*(*bufcapacity);
    void *newbuf;
    if (newcapacity < incl_len)
    {
      newcapacity = incl_len;
    }
    if (newcapacity < 1514)
    {
      newcapacity = 1514;
    }
    newbuf = realloc(*buf, newcapacity);
    if (newbuf == NULL)
    {
      return -ENOMEM;
    }
    *buf = newbuf;
    *bufcapacity = newcapacity;
  }
  if (fread(*buf, incl_len, 1, ctx->f) != 1)
  {
    return -EINVAL;
  }
  if (time64)
  {
    *time64 = ((uint64_t)sec)*1000*1000 + usec;
  }
  return 1;
}

int pcap_out_ctx_init(
  struct pcap_out_ctx *ctx, const char *fname)
{
  FILE *f = fopen(fname, "wb");
  char hdr[24];
  if (f == NULL)
  {
    return -EPERM;
  }
  hdr_set32h(&hdr[0], 0xa1b2c3d4);
  hdr_set16h(&hdr[4], 2);
  hdr_set16h(&hdr[6], 4);
  hdr_set32h(&hdr[8], 0);
  hdr_set32h(&hdr[12], 0);
  hdr_set32h(&hdr[16], 0xFFFFFFFFU);
  hdr_set32h(&hdr[20], 1);
  if (fwrite(hdr, 24, 1, f) != 1)
  {
    fclose(f);
    return -EIO;
  }
  ctx->f = f;
  return 0;
}

int pcap_out_ctx_write(
  struct pcap_out_ctx *ctx, void *buf, size_t size, uint64_t time64)
{
  uint32_t sec, usec;
  char hdr[16];
  if (time64 == 0)
  {
    time64 = gettime64();
  }
  sec = time64/1000/1000;
  usec = time64%(1000*1000);
  hdr_set32h(&hdr[0], sec);
  hdr_set32h(&hdr[4], usec);
  hdr_set32h(&hdr[8], size);
  hdr_set32h(&hdr[12], size);
  if (fwrite(hdr, 16, 1, ctx->f) != 1)
  {
    return -EIO;
  }
  if (fwrite(buf, size, 1, ctx->f) != 1)
  {
    return -EIO;
  }
  if (fflush(ctx->f) != 0)
  {
    return -EIO;
  }
  return 0;
}

int pcap_in_ctx_init_file(
  struct pcap_in_ctx *ctx, FILE *f, int enforce_ethernet)
{
  char hdr[24];
  uint16_t version_major, version_minor;
  uint32_t network;
  if (f == NULL)
  {
    return -ENOENT;
  }
  if (fread(hdr, 24, 1, f) != 1)
  {
    fclose(f);
    return -EINVAL;
  }
  if (hdr_get32h(&hdr[0]) == 0xa1b2c3d4)
  {
    ctx->swap = 0;
    ctx->ns = 0;
  }
  else if (hdr_get32h(&hdr[0]) == 0xd4c3b2a1)
  {
    ctx->swap = 1;
    ctx->ns = 0;
  }
  else if (hdr_get32h(&hdr[0]) == 0xa1b23c4d)
  {
    ctx->swap = 0;
    ctx->ns = 1;
  }
  else if (hdr_get32h(&hdr[0]) == 0x4d3cb2a1)
  {
    ctx->swap = 1;
    ctx->ns = 1;
  }
  else
  {
    fclose(f);
    return -EINVAL;
  }
  version_major = hdr_get16h(&hdr[4]);
  if (ctx->swap)
  {
    version_major = byteswap16(version_major);
  }
  version_minor = hdr_get16h(&hdr[6]);
  if (ctx->swap)
  {
    version_minor = byteswap16(version_minor);
  }
  if (version_major != 2 || version_minor != 4)
  {
    fclose(f);
    return -EINVAL;
  }
  unsigned thiszonetmp = hdr_get32h(&hdr[8]);
  if (thiszonetmp > 0)
  {
    fclose(f);
    return -EINVAL;
  }
  if (ctx->swap)
  {
    thiszonetmp = byteswap32(thiszonetmp);
  }
  ctx->thiszone = (int)thiszonetmp;
  network = hdr_get32h(&hdr[20]);
  if (ctx->swap)
  {
    network = byteswap32(network);
  }
  if (enforce_ethernet && network != 1)
  {
    fclose(f);
    return -EINVAL;
  }
  ctx->f = f;
  
  return 0;
}

int pcap_in_ctx_init(
  struct pcap_in_ctx *ctx, const char *fname, int enforce_ethernet)
{
  FILE *f = fopen(fname, "rb");
  return pcap_in_ctx_init_file(ctx, f, enforce_ethernet);
}
