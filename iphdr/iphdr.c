#include "iphdr.h"
#include <errno.h>

void tcp_parse_options(
  void *pkt,
  struct tcp_information *info)
{
  char *cpkt = pkt;
  size_t dataoff = tcp_data_offset(pkt);
  size_t curoff = 20;
  size_t curoptlen;
  info->mss = 536;
  info->wscale = 0;
  info->options_valid = 0;
  info->sack_permitted = 0;
  info->mssoff = 0;
  info->ts_present = 0;
  info->ts = 0;
  info->tsecho = 0;
  while (curoff < dataoff)
  {
    if (cpkt[curoff] == 0)
    {
      info->options_valid = 1;
      return;
    }
    if (cpkt[curoff] == 1)
    {
      curoff++;
      continue;
    }
    if (cpkt[curoff] == 8)
    {
      size_t tslenval = dataoff - curoff;
      if (curoff + 1 < dataoff && ((unsigned char)cpkt[curoff + 1]) < tslenval)
      {
        tslenval = (unsigned char)cpkt[curoff + 1];
      }
      if (tslenval < 2)
      {
        return;
      }
      if (tslenval != 10)
      {
        curoff += tslenval;
        continue;
      }
      info->ts = hdr_get32n(&cpkt[curoff + 2]);
      info->tsecho = hdr_get32n(&cpkt[curoff + 6]);
      info->ts_present = 1;
      curoff += tslenval;
      continue;
    }
    if (cpkt[curoff] == 3)
    {
      size_t wscalelenval = dataoff - curoff;
      if (curoff + 1 < dataoff && ((unsigned char)cpkt[curoff + 1]) < wscalelenval)
      {
        wscalelenval = (unsigned char)cpkt[curoff + 1];
      }
      if (wscalelenval < 2)
      {
        return;
      }
      if (wscalelenval != 3)
      {
        curoff += wscalelenval;
        continue;
      }
      info->wscale = cpkt[curoff + 2];
      curoff += wscalelenval;
      continue;
    }
    if (cpkt[curoff] == 2)
    {
      size_t msslenval = dataoff - curoff;
      if (curoff + 1 < dataoff && ((unsigned char)cpkt[curoff + 1]) < msslenval)
      {
        msslenval = (unsigned char)cpkt[curoff + 1];
      }
      if (msslenval < 2)
      {
        return;
      }
      if (msslenval != 4)
      {
        curoff += msslenval;
        continue;
      }
      info->mss = hdr_get16n(&cpkt[curoff + 2]);
      info->mssoff = curoff;
      curoff += msslenval;
      continue;
    }
    if (cpkt[curoff] == 4)
    {
      size_t sacklenval = dataoff - curoff;
      if (curoff + 1 < dataoff && ((unsigned char)cpkt[curoff + 1]) < sacklenval)
      {
        sacklenval = (unsigned char)cpkt[curoff + 1];
      }
      if (sacklenval < 2)
      {
        return;
      }
      if (sacklenval != 2)
      {
        curoff += sacklenval;
        continue;
      }
      info->sack_permitted = 1;
      curoff += sacklenval;
      continue;
    }
    if (curoff + 1 >= dataoff)
    {
      return;
    }
    curoptlen = dataoff - curoff;
    if ((unsigned char)cpkt[curoff + 1] < curoptlen)
    {
      curoptlen = (unsigned char)cpkt[curoff + 1];
    }
    if (curoptlen < 2)
    {
      return;
    }
    curoff += curoptlen;
  }
  info->options_valid = 1;
  return;
}

void tcp_find_sack_ts_headers(
  void *pkt, struct sack_ts_headers *hdrs)
{
  char *cpkt = pkt;
  size_t dataoff = tcp_data_offset(pkt);
  size_t curoff = 20;
  size_t curoptlen;
  hdrs->sackoff = 0;
  hdrs->sacklen = 0;
  hdrs->tsoff = 0;
  while (curoff < dataoff)
  {
    if (cpkt[curoff] == 0)
    {
      return;
    }
    if (cpkt[curoff] == 1)
    {
      curoff++;
      continue;
    }
    if (cpkt[curoff] == 5)
    {
      size_t sacklenval = dataoff - curoff;
      if (curoff + 1 < dataoff && ((unsigned char)cpkt[curoff + 1]) < sacklenval)
      {
        sacklenval = (unsigned char)cpkt[curoff + 1];
      }
      hdrs->sacklen = sacklenval;
      hdrs->sackoff = curoff;
      curoff += sacklenval;
      continue;
    }
    if (cpkt[curoff] == 8)
    {
      size_t tslenval = dataoff - curoff;
      if (curoff + 1 < dataoff && ((unsigned char)cpkt[curoff + 1]) < tslenval)
      {
        tslenval = (unsigned char)cpkt[curoff + 1];
      }
      if (tslenval != 10)
      {
        curoff += tslenval;
        continue;
      }
      hdrs->tsoff = curoff;
      curoff += tslenval;
      continue;
    }
    if (curoff + 1 >= dataoff)
    {
      return;
    }
    curoptlen = dataoff - curoff;
    if ((unsigned char)cpkt[curoff + 1] < curoptlen)
    {
      curoptlen = (unsigned char)cpkt[curoff + 1];
    }
    if (curoptlen < 2)
    {
      return;
    }
    curoff += curoptlen;
  }
  return;
}

void *tcp_find_sack_header(
  void *pkt, size_t *sacklen, int *sixteen_bit_align)
{
  char *cpkt = pkt;
  size_t dataoff = tcp_data_offset(pkt);
  size_t curoff = 20;
  size_t curoptlen;
  while (curoff < dataoff)
  {
    if (cpkt[curoff] == 0)
    {
      return NULL;
    }
    if (cpkt[curoff] == 1)
    {
      curoff++;
      continue;
    }
    if (cpkt[curoff] == 5)
    {
      size_t sacklenval = dataoff - curoff;
      if (curoff + 1 < dataoff && ((unsigned char)cpkt[curoff + 1]) < sacklenval)
      {
        sacklenval = (unsigned char)cpkt[curoff + 1];
      }
      if (sacklen)
      {
        *sacklen = sacklenval;
      }
      if (sixteen_bit_align)
      {
        *sixteen_bit_align = !(curoff%2);
      }
      return &cpkt[curoff];
    }
    if (curoff + 1 >= dataoff)
    {
      return NULL;
    }
    curoptlen = dataoff - curoff;
    if ((unsigned char)cpkt[curoff + 1] < curoptlen)
    {
      curoptlen = (unsigned char)cpkt[curoff + 1];
    }
    if (curoptlen < 2)
    {
      return NULL;
    }
    curoff += curoptlen;
  }
  return NULL;
}

int dns_put_next(void *vdns, uint16_t *off, uint16_t *remcnt,
                 uint16_t plen,
                 char *buf,
                 uint16_t qtype, uint16_t qclass, uint32_t ttl,
                 uint16_t rdlength, void *rdata)
{
  unsigned char *cdns = vdns;
  char *chr;
  if (*off + strlen(buf)+2 + 10 + rdlength > plen)
  {
    return -EFAULT;
  }
  for (;;)
  {
    chr = strchr(buf, '.');
    if (chr == NULL)
    {
      chr = buf+strlen(buf);
    }
    if (*off + 1 + (chr-buf) > plen)
    {
      return -EFAULT;
    }
    cdns[(*off)++] = chr-buf;
    memcpy(&cdns[*off], buf, chr-buf);
    (*off) += chr-buf;
    if (chr == buf)
    {
      break;
    }
    if (*chr == '\0')
    {
      buf = chr;
    }
    else
    {
      buf = chr+1;
    }
  }
  if (*off + 10 + rdlength > plen)
  {
    return -EFAULT;
  }
  hdr_set16n(&cdns[(*off)], qtype);
  hdr_set16n(&cdns[(*off)+2], qclass);
  hdr_set32n(&cdns[(*off)+4], ttl);
  hdr_set16n(&cdns[(*off)+8], rdlength);
  memcpy(&cdns[(*off)+10], rdata, rdlength);
  (*off) += 10+rdlength;
  return 0;
}

int dns_put_next_qr(void *vdns, uint16_t *off, uint16_t *remcnt,
                    uint16_t plen,
                    char *buf,
                    uint16_t qtype, uint16_t qclass)
{
  unsigned char *cdns = vdns;
  char *chr;
  if (*off + strlen(buf)+2 + 4 > plen)
  {
    return -EFAULT;
  }
  for (;;)
  {
    chr = strchr(buf, '.');
    if (chr == NULL)
    {
      chr = buf+strlen(buf);
    }
    if (*off + 1 + (chr-buf) > plen)
    {
      return -EFAULT;
    }
    cdns[(*off)++] = chr-buf;
    memcpy(&cdns[*off], buf, chr-buf);
    (*off) += chr-buf;
    if (chr == buf)
    {
      break;
    }
    if (*chr == '\0')
    {
      buf = chr;
    }
    else
    {
      buf = chr+1;
    }
  }
  if (*off + 4 > plen)
  {
    return -EFAULT;
  }
  hdr_set16n(&cdns[(*off)], qtype);
  hdr_set16n(&cdns[(*off)+2], qclass);
  (*off) += 4;
  return 0;
}

int dns_next(void *vdns, uint16_t *off, uint16_t *remcnt,
             uint16_t plen,
             char *buf, size_t bufsiz, uint16_t *qtype,
             uint16_t *qclass)
{
  unsigned char *cdns = vdns;
  uint16_t tocopy;
  uint16_t labsiz, laboff;
  size_t strlentmp;
  *buf = '\0';
  if (*remcnt == 0)
  {
    return -ENOENT;
  }
  while (*off < plen && *remcnt)
  {
    labsiz = cdns[(*off)++];
    if (labsiz == 0)
    {
      break;
    }
    if ((labsiz & 0xc0) == 0xc0)
    {
      labsiz &= ~0xc0;
      labsiz <<= 8;
      if (*off >= plen)
      {
        return -EFAULT;
      }
      labsiz |= cdns[(*off)++];
      if (labsiz >= plen)
      {
        return -EFAULT;
      }
      laboff = labsiz + 1;
      labsiz = cdns[laboff - 1];
    }
    else
    {
      laboff = *off;
    }
    if (laboff + labsiz >= plen) // have to have room for '\0'
    {
      return -EFAULT;
    }
    tocopy = labsiz;
    strlentmp = strlen(buf);
    if (tocopy+strlentmp+2 > bufsiz)
    {
      return -EFAULT;
    }
    (*off) += labsiz;
    memcpy(buf+strlentmp, &cdns[laboff], tocopy);
    memcpy(buf+strlentmp+tocopy, ".\0", 2);
    strlentmp += tocopy + 1;
  }
  if (*off + 4 > plen)
  {
    return -EFAULT;
  }
  *qtype = hdr_get16n(&cdns[(*off)]);
  *qclass = hdr_get16n(&cdns[(*off)+2]);
  (*off) += 4;
  strlentmp = strlen(buf);
  buf[strlentmp-1] = '\0';
  (*remcnt)--;
  return 0;
}
