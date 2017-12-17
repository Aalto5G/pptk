#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "byteswap.h"
#include "hdr.h"
#include "mypcapng.h"
#include "containerof.h"
#include "siphash.h"
#include "time64.h"

static ssize_t fskip(size_t sz, FILE *f)
{
  char buf[512];
  size_t skipped = 0;
  while (skipped < sz)
  {
    size_t remaining = sz - skipped;
    ssize_t ret;
    ret = fread(buf, 1, (remaining > 512) ? 512 : remaining, f);
    if (ret <= 0)
    {
      if (ret < 0 && skipped == 0)
      {
        return ret;
      }
      return skipped;
    }
    skipped += ret;
  }
  return skipped;
}

static inline uint32_t string_hash(const char *str)
{
  struct siphash_ctx ctx;
  const char key[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
  siphash_init(&ctx, key);
  if (str == NULL)
  {
    return 0;
  }
  // slow but sure way...
  while (*str)
  {
    siphash_feed_u64(&ctx, (uint8_t)*str);
    str++;
  }
  return siphash_get(&ctx);
}

static uint32_t entry_hash_fn(struct hash_list_node *e, void *userdata)
{
  struct pcapng_out_interface *out;
  out = CONTAINER_OF(e, struct pcapng_out_interface, node);
  return string_hash(out->name);
}

static void free_interfaces(struct pcapng_in_ctx *ctx)
{
  size_t sz = DYNARR_SIZE(&ctx->interfaces);
  size_t i;
  for (i = 0; i < sz; i++)
  {
    struct pcapng_in_interface *intf = &DYNARR_GET(&ctx->interfaces, i);
    free(intf->name);
    intf->name = NULL;
  }
  DYNARR_CLEAR(&ctx->interfaces);
}

void pcapng_in_ctx_free(struct pcapng_in_ctx *ctx)
{
  free_interfaces(ctx);
  DYNARR_FREE(&ctx->interfaces);
  fclose(ctx->f);
  ctx->f = NULL;
}

void pcapng_out_ctx_free(struct pcapng_out_ctx *ctx)
{
  size_t bucket;
  struct hash_list_node *n, *x;
  HASH_TABLE_FOR_EACH_SAFE(&ctx->hash, bucket, n, x)
  {
    struct pcapng_out_interface *out;
    out = CONTAINER_OF(n, struct pcapng_out_interface, node);
    hash_table_delete(&ctx->hash, &out->node, string_hash(out->name));
    free(out->name);
    out->name = NULL;
    free(out);
  }
  hash_table_free(&ctx->hash);
  fclose(ctx->f);
  ctx->f = NULL;
}

int pcapng_out_ctx_init(
  struct pcapng_out_ctx *ctx, const char *fname)
{
  FILE *f = fopen(fname, "wb");
  char hdr[28];
  if (f == NULL)
  {
    return -EPERM;
  }
  ctx->f = f;
  ctx->next_index = 0;
  hash_table_init(&ctx->hash, 1024, entry_hash_fn, NULL);
  hdr_set32h(&hdr[0], 0x0A0D0D0A);
  hdr_set32h(&hdr[4], 28);
  hdr_set32h(&hdr[8], 0x1A2B3C4D);
  hdr_set16h(&hdr[12], 1);
  hdr_set16h(&hdr[14], 0);
  hdr_set64h(&hdr[16], 0xFFFFFFFFFFFFFFFFULL);
  hdr_set32h(&hdr[24], 28);
  if (fwrite(hdr, 28, 1, ctx->f) != 1)
  {
    return -EIO;
  }
  return 0;
}

static int pcapng_write_ifname(
  struct pcapng_out_ctx *ctx, struct pcapng_out_interface *intf)
{
  char hdr[20];
  char pad[3] = {0};
  size_t namelen = strlen(intf->name);
  uint32_t namelenpadded = (namelen+3)/4*4;
  hdr_set32h(&hdr[0], 0x00000001);
  hdr_set32h(&hdr[4], 20 + 8 + namelenpadded);
  hdr_set16h(&hdr[8], 1); // link type
  hdr_set16h(&hdr[10], 0); // reserved
  hdr_set32h(&hdr[12], 0xFFFFFFFFU); // snap len
  hdr_set16h(&hdr[16], 2); // if_name
  hdr_set16h(&hdr[18], namelen);
  if (fwrite(hdr, 20, 1, ctx->f) != 1)
  {
    return -EIO;
  }
  if (fwrite(intf->name, namelen, 1, ctx->f) != 1)
  {
    return -EIO;
  }
  if (namelenpadded - namelen > 0)
  {
    if (fwrite(pad, namelenpadded - namelen, 1, ctx->f) != 1)
    {
      return -EIO;
    }
  }
  hdr_set16h(&hdr[0], 0);
  hdr_set16h(&hdr[2], 0);
  hdr_set32h(&hdr[4], 20 + 8 + namelenpadded);
  if (fwrite(hdr, 8, 1, ctx->f) != 1)
  {
    return -EIO;
  }
  return 0;
}

int pcapng_out_ctx_write(
  struct pcapng_out_ctx *ctx, void *buf, size_t len, uint64_t time64,
  const char *ifname)
{
  char pad[3] = {0};
  char hdr[28];
  uint32_t len_padded = (len+3)/4*4;
  uint32_t tlen = len_padded + 32;
  uint32_t hashval = string_hash(ifname);
  struct hash_list_node *n;
  struct pcapng_out_interface *intf;
  int ret;
  HASH_TABLE_FOR_EACH_POSSIBLE(&ctx->hash, n, hashval)
  {
    intf = CONTAINER_OF(n, struct pcapng_out_interface, node);
    if (strcmp(intf->name, ifname) == 0)
    {
      goto found;
    }
  }
  intf = malloc(sizeof(*intf));
  if (intf == NULL)
  {
    return -ENOMEM;
  }
  intf->name = strdup(ifname);
  if (intf->name == NULL)
  {
    free(intf);
    return -ENOMEM;
  }
  intf->index = ctx->next_index++;
  hash_table_add_nogrow(&ctx->hash, &intf->node, hashval);
  ret = pcapng_write_ifname(ctx, intf);
  if (ret != 0)
  {
    printf("write ifname failed\n");
    return ret;
  }
found:
  hdr_set32h(&hdr[0], 0x00000006);
  hdr_set32h(&hdr[4], tlen);
  hdr_set32h(&hdr[8], intf->index);
  if (time64 == 0)
  {
    time64 = gettime64();
  }
  hdr_set32h(&hdr[12], (uint32_t)(time64>>32));
  hdr_set32h(&hdr[16], (uint32_t)(time64&0xFFFFFFFFU));
  hdr_set32h(&hdr[20], len);
  hdr_set32h(&hdr[24], len);
  if (fwrite(hdr, 28, 1, ctx->f) != 1)
  {
    return -EIO;
  }
  if (fwrite(buf, len, 1, ctx->f) != 1)
  {
    return -EIO;
  }
  if (len_padded - len > 0)
  {
    if (fwrite(pad, len_padded - len, 1, ctx->f) != 1)
    {
      return -EIO;
    }
  }
  hdr_set32h(&hdr[0], tlen);
  if (fwrite(hdr, 4, 1, ctx->f) != 1)
  {
    return -EIO;
  }
  if (fflush(ctx->f) != 0)
  {
    return -EIO;
  }
  return 0;
}

static int pcapng_in_ctx_read_shb(struct pcapng_in_ctx *ctx)
{
  char hdr[20];
  uint32_t tlen;
  uint32_t tlen2;
  uint32_t bom;
  uint16_t major;
  free_interfaces(ctx);
  if (fread(hdr, 20, 1, ctx->f) != 1)
  {
    return -EINVAL;
  }
  bom = hdr_get32h(&hdr[4]);
  if (bom == 0x1A2B3C4D)
  {
    ctx->swap = 0;
  }
  else if (bom == 0x4D3C2B1A)
  {
    ctx->swap = 1;
  }
  else
  {
    return -EINVAL;
  }
  tlen = hdr_get32h(hdr);
  if (ctx->swap)
  {
    tlen = byteswap32(tlen);
  }
  if (tlen < 28 || (tlen % 4) != 0)
  {
    return -EINVAL;
  }
  major = hdr_get16h(&hdr[8]);
  if (ctx->swap)
  {
    major = byteswap16(major);
  }
  if (major != 1)
  {
    return -ENOTSUP;
  }
  if (fskip(tlen-28, ctx->f) != (ssize_t)tlen-28)
  {
    return -EINVAL;
  }
  if (fread(hdr, 4, 1, ctx->f) != 1)
  {
    return -EINVAL;
  }
  tlen2 = hdr_get32h(hdr);
  if (ctx->swap)
  {
    tlen2 = byteswap32(tlen2);
  }
  if (tlen2 != tlen)
  {
    return -EINVAL;
  }
  return 0;
}

static int pcapng_in_ctx_read_idb(struct pcapng_in_ctx *ctx)
{
  char hdr[20];
  uint32_t tlen;
  uint32_t tlen2;
  struct pcapng_in_interface *intf;
  uint32_t remaining_opts;
  if (fread(hdr, 12, 1, ctx->f) != 1)
  {
    return -EINVAL;
  }
  tlen = hdr_get32h(hdr);
  if (ctx->swap)
  {
    tlen = byteswap32(tlen);
  }
  if (tlen < 20 || (tlen % 4) != 0)
  {
    return -EINVAL;
  }
  intf = DYNARR_PUSH_BACK_RETPTR(&ctx->interfaces);
  if (intf == NULL)
  {
    return -ENOMEM;
  }
  intf->linktype = hdr_get16h(&hdr[4]);
  if (ctx->swap)
  {
    intf->linktype = byteswap16(intf->linktype);
  }
  intf->snaplen = hdr_get32h(&hdr[8]);
  if (ctx->swap)
  {
    intf->snaplen = byteswap32(intf->snaplen);
  }
  intf->name = NULL;
  intf->tsresol = 6;
  remaining_opts = tlen - 20;
  while (remaining_opts >= 4)
  {
    uint16_t optcode, optlen;
    uint32_t thisoptlen;
    if (fread(hdr, 4, 1, ctx->f) != 1)
    {
      return -EINVAL;
    }
    optcode = hdr_get16h(&hdr[0]);
    optlen = hdr_get16h(&hdr[2]);
    if (ctx->swap)
    {
      optcode = byteswap16(optcode);
      optlen = byteswap16(optlen);
    }
    remaining_opts -= 4;
    thisoptlen = (optlen+3)/4*4;
    if (thisoptlen > remaining_opts)
    {
      return -EINVAL;
    }
    remaining_opts -= thisoptlen;
    if (optcode == 9)
    {
      uint8_t tsresol;
      if (optlen != 1)
      {
        return -EINVAL;
      }
      if (fread(hdr, 4, 1, ctx->f) != 1)
      {
        return -EINVAL;
      }
      tsresol = hdr_get8h(&hdr[0]);
      intf->tsresol = tsresol;
    }
    else if (optcode == 14)
    {
      uint64_t tsoffset;
      if (optlen != 8)
      {
        return -EINVAL;
      }
      if (fread(hdr, 8, 1, ctx->f) != 1)
      {
        return -EINVAL;
      }
      tsoffset = hdr_get64h(&hdr[0]);
      if (ctx->swap)
      {
        tsoffset = byteswap64(tsoffset);
      }
      intf->tsoffset = (int64_t)tsoffset;
    }
    else if (optcode == 2)
    {
      char *buf;
      if (intf->name != NULL)
      {
        return -EINVAL;
      }
      buf = malloc(thisoptlen + 1);
      if (buf == NULL)
      {
        return -ENOMEM;
      }
      if (fread(buf, thisoptlen, 1, ctx->f) != 1)
      {
        return -EINVAL;
      }
      buf[optlen] = '\0';
      intf->name = buf;
    }
    else
    {
      if (fskip(thisoptlen, ctx->f) != (ssize_t)thisoptlen)
      {
        return -EINVAL;
      }
    }
  }
  if (fread(hdr, 4, 1, ctx->f) != 1)
  {
    return -EINVAL;
  }
  tlen2 = hdr_get32h(hdr);
  if (ctx->swap)
  {
    tlen2 = byteswap32(tlen2);
  }
  if (tlen2 != tlen)
  {
    return -EINVAL;
  }
  return 0;
}



int pcapng_in_ctx_read(
  struct pcapng_in_ctx *ctx, void **buf, size_t *bufcapacity,
  size_t *len, size_t *snap, uint64_t *time64, const char **ifname)
{
  char hdr[28];
  uint32_t btype;
  uint32_t tshi, tslo;
  uint64_t ts;
  uint32_t ifid;
  uint32_t captured_len, orig_len;
  uint32_t packet_data_len;
  uint32_t padding;
  uint32_t tlen, tlen2;
  uint32_t toskip;
  for (;;)
  {
    if (fread(hdr, 4, 1, ctx->f) != 1)
    {
      if (feof(ctx->f))
      {
        return 0;
      }
      return -EINVAL;
    }
    btype = hdr_get32h(&hdr[0]);
    if (ctx->swap)
    {
      btype = byteswap32(btype);
    }
    if (btype == 0x0A0D0D0A)
    {
      int ret = pcapng_in_ctx_read_shb(ctx);
      if (ret != 0)
      {
        return ret;
      }
      continue;
    }
    else if (btype == 0x00000001)
    {
      int ret = pcapng_in_ctx_read_idb(ctx);
      if (ret != 0)
      {
        return ret;
      }
      continue;
    }
    else if (btype != 0x00000006 && btype != 0x00000003)
    {
      if (fread(hdr, 4, 1, ctx->f) != 1)
      {
        return -EINVAL;
      }
      tlen = hdr_get32h(&hdr[0]);
      if (ctx->swap)
      {
        tlen = byteswap32(tlen);
      }
      if (tlen < 12)
      {
        return -EINVAL;
      }
      if (fskip(tlen - 12, ctx->f) != (ssize_t)tlen - 12)
      {
        return -EINVAL;
      }
      if (fread(hdr, 4, 1, ctx->f) != 1)
      {
        return -EINVAL;
      }
      tlen2 = hdr_get32h(&hdr[0]);
      if (ctx->swap)
      {
        tlen2 = byteswap32(tlen2);
      }
      if (tlen2 != tlen)
      {
        return -EINVAL;
      }
      continue;
    }
    else
    {
      break;
    }
  }
  if (btype == 0x00000003)
  {
    uint32_t snaplen;
    if (fread(hdr, 8, 1, ctx->f) != 1)
    {
      return -EINVAL;
    }
    tlen = hdr_get32h(&hdr[0]);
    if (ctx->swap)
    {
      tlen = byteswap32(tlen);
    }
    if (tlen < 16)
    {
      return -EINVAL;
    }
    ifid = 0;
    if (ifid >= DYNARR_SIZE(&ctx->interfaces))
    {
      return -EINVAL;
    }
    orig_len = hdr_get32h(&hdr[4]);
    snaplen = DYNARR_GET(&ctx->interfaces, ifid).snaplen;
    if (orig_len > snaplen)
    {
      captured_len = snaplen;
    }
    else
    {
      captured_len = orig_len;
    }
    packet_data_len = (captured_len+3)/4 * 4;
    if (packet_data_len != tlen - 16)
    {
      return -EINVAL;
    }
    padding = packet_data_len - captured_len;
    if (captured_len > *bufcapacity || *buf == NULL)
    {
      size_t newcapacity = 2*(*bufcapacity);
      void *newbuf;
      if (newcapacity < captured_len)
      {
        newcapacity = captured_len;
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
    if (fread(*buf, captured_len, 1, ctx->f) != 1)
    {
      return -EINVAL;
    }
    if (fskip(padding, ctx->f) != (ssize_t)padding)
    {
      return -EINVAL;
    }
    if (len)
    {
      *len = orig_len;
    }
    if (snap)
    {
      *snap = captured_len;
    }
    if (ifname)
    {
      *ifname = DYNARR_GET(&ctx->interfaces, ifid).name;
    }
    if (time64)
    {
      *time64 = ctx->lasttime;
    }
    return 0;
  }
  if (fread(hdr, 24, 1, ctx->f) != 1)
  {
    return -EINVAL;
  }
  tlen = hdr_get32h(&hdr[0]);
  if (ctx->swap)
  {
    tlen = byteswap32(tlen);
  }
  if (tlen < 32)
  {
    return -EINVAL;
  }
  ifid = hdr_get32h(&hdr[4]);
  if (ctx->swap)
  {
    ifid = byteswap32(ifid);
  }
  if (ifid >= DYNARR_SIZE(&ctx->interfaces))
  {
    return -EINVAL;
  }
  tshi = hdr_get32h(&hdr[8]);
  tslo = hdr_get32h(&hdr[12]);
  captured_len = hdr_get32h(&hdr[16]);
  orig_len = hdr_get32h(&hdr[20]);
  if (ctx->swap)
  {
    tshi = byteswap32(tshi);
    tslo = byteswap32(tslo);
    captured_len = byteswap32(captured_len);
    orig_len = byteswap32(orig_len);
  }
  ts = (((uint64_t)tshi)<<32) | tslo;
  packet_data_len = (captured_len+3)/4 * 4;
  if (packet_data_len < tlen - 32)
  {
    return -EINVAL;
  }
  padding = packet_data_len - captured_len;
  if (captured_len > *bufcapacity || *buf == NULL)
  {
    size_t newcapacity = 2*(*bufcapacity);
    void *newbuf;
    if (newcapacity < captured_len)
    {
      newcapacity = captured_len;
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
  if (fread(*buf, captured_len, 1, ctx->f) != 1)
  {
    return -EINVAL;
  }
  if (fskip(padding, ctx->f) != (ssize_t)padding)
  {
    return -EINVAL;
  }
  toskip = packet_data_len - (tlen - 32);
  if (fskip(toskip, ctx->f) != (ssize_t)toskip)
  {
    return -EINVAL;
  }
  if (fread(hdr, 4, 1, ctx->f) != 1)
  {
    return -EINVAL;
  }
  tlen2 = hdr_get32h(&hdr[0]);
  if (ctx->swap)
  {
    tlen2 = byteswap32(tlen2);
  }
  if (tlen2 != tlen)
  {
    return -EINVAL;
  }
  if (1)
  {
    uint64_t time64val;
    struct pcapng_in_interface *intf = &DYNARR_GET(&ctx->interfaces, ifid);
    if (intf->tsresol & 0x80)
    {
      uint8_t rem = intf->tsresol & (~0x80);
      double x = ts;
      int cnt = rem;
      while (cnt > 0)
      {
        cnt--;
        x /= 2;
      }
      x *= (1000*1000);
      x += 0.5;
      time64val = ((uint64_t)x) + intf->tsoffset;
    }
    else
    {
      if (intf->tsresol > 6)
      {
        int cnt = intf->tsresol - 6;
        while (cnt > 0)
        {
          cnt--;
          ts /= 10;
        }
      }
      else if (intf->tsresol < 6)
      {
        int cnt = 6 - intf->tsresol;
        while (cnt > 0)
        {
          cnt--;
          ts *= 10;
        }
      }
      time64val = ts + intf->tsoffset;
    }
    ctx->lasttime = time64val;
    if (time64)
    {
      *time64 = time64val;
    }
  }
  if (len)
  {
    *len = orig_len;
  }
  if (snap)
  {
    *snap = captured_len;
  }
  if (ifname)
  {
    *ifname = DYNARR_GET(&ctx->interfaces, ifid).name;
  }
  return 1;
}

int pcapng_in_ctx_init(
  struct pcapng_in_ctx *ctx, const char *fname, int enforce_ethernet)
{
  FILE *f = fopen(fname, "rb");
  if (f == NULL)
  {
    return -ENOENT;
  }
  ctx->f = f;
  ctx->lasttime = gettime64();
  ctx->swap = 0; // have to put some value to silence Valgrind
  DYNARR_INIT(&ctx->interfaces);
  
  return 0;
}
