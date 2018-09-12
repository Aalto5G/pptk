#include "dnshdr.h"
#include "hdr.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int dns_next_an_ask(const void *vdns, uint16_t *off, uint16_t *remcnt,
                    uint16_t plen,
                    char *buf, size_t bufsiz, uint16_t *qtype,
                    uint16_t *qclass, uint32_t *ttl,
                    char *datbuf, size_t datbufsiz, size_t *datbuflen)
{
  const unsigned char *cdns = vdns;
  uint16_t tocopy;
  uint16_t labsiz, laboff;
  uint16_t datsiz, datoff, datrem;
  size_t strlentmp;
  int jmp = 0;
  *buf = '\0';
  *datbuf = '\0';
  if (*remcnt == 0)
  {
    return -ENOENT;
  }
  laboff = *off;
  while (*off < plen && *remcnt)
  {
    labsiz = cdns[laboff++];
    //printf("labsiz %d at off %d\n", (int)labsiz, (int)laboff);
    if (!jmp)
    {
      (*off)++;
    }
    if (labsiz == 0)
    {
      break;
    }
    if ((labsiz & 0xc0) == 0xc0)
    {
      labsiz &= ~0xc0;
      labsiz <<= 8;
      if (laboff >= plen)
      {
        return -EFAULT;
      }
      labsiz |= cdns[laboff++];
      if (!jmp)
      {
        (*off)++;
      }
      if (labsiz >= plen)
      {
        return -EFAULT;
      }
      laboff = labsiz + 1;
      labsiz = cdns[laboff - 1];
      jmp = 1;
    }
    else
    {
      //laboff = *off;
      if (!jmp)
      {
        (*off) += labsiz;
      }
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
    memcpy(buf+strlentmp, &cdns[laboff], tocopy);
    memcpy(buf+strlentmp+tocopy, ".\0", 2);
    //printf("buf now %s\n", buf);
    strlentmp += (size_t)tocopy + 1;
    laboff += labsiz;
  }
  if (*off + 10 > plen)
  {
    return -EFAULT;
  }
  *qtype = hdr_get16n(&cdns[(*off)]);
  *qclass = hdr_get16n(&cdns[(*off)+2]);
  *ttl = hdr_get32n(&cdns[(*off)+4]);
  datrem = hdr_get16n(&cdns[(*off)+8]);
  (*off) += 10;
  if (*qtype != 5)
  {
    uint16_t tocopy2 = datrem;
    if (((size_t)tocopy2+2) > datbufsiz)
    {
      return -EFAULT;
    }
    memcpy(datbuf, &cdns[*off], tocopy2);
    *datbuflen = datrem;
  }
  else
  {
    datoff = *off;
    jmp = 0;
    while (*off < plen && *remcnt)
    {
      if (!datrem)
      {
        return -EFAULT;
      }
      datsiz = cdns[datoff++];
      //printf("datsiz %d at off %d\n", (int)datsiz, (int)datoff);
      if (!jmp)
      {
        (*off)++;
      }
      if (datsiz == 0)
      {
        break;
      }
      if ((datsiz & 0xc0) == 0xc0)
      {
        datsiz &= ~0xc0;
        datsiz <<= 8;
        if (datoff >= plen)
        {
          return -EFAULT;
        }
        datsiz |= cdns[datoff++];
        if (!jmp)
        {
          (*off)++;
        }
        if (datsiz >= plen)
        {
          return -EFAULT;
        }
        datoff = datsiz + 1;
        datsiz = cdns[datoff - 1];
        jmp = 1;
      }
      else
      {
        //datoff = *off;
        if (!jmp)
        {
          (*off) += datsiz;
        }
      }
      if (datoff + datsiz >= plen) // have to have room for '\0'
      {
        return -EFAULT;
      }
      tocopy = datsiz;
      strlentmp = strlen(datbuf);
      if (tocopy+strlentmp+2 > bufsiz)
      {
        return -EFAULT;
      }
      memcpy(datbuf+strlentmp, &cdns[datoff], tocopy);
      memcpy(datbuf+strlentmp+tocopy, ".\0", 2);
      //printf("datbuf now %s\n", datbuf);
      strlentmp += (size_t)tocopy + 1;
      datoff += datsiz;
    }
    strlentmp = strlen(datbuf);
    *datbuflen = strlentmp;
    datbuf[strlentmp-1] = '\0';
  }
  (*remcnt)--;
  return 0;
}

static ssize_t memdns(void *mem, size_t off, char *dns)
{
  size_t cstrlen = strlen(dns);
  size_t dnsstrlen = (cstrlen == 0) ? 1 : (cstrlen + 2);
  size_t i;
  uint8_t u8;
  char *buf = (void*)mem;
  if (dnsstrlen > off)
  {
    return -1;
  }
#if 0
  // Print the whole buffer to catch uninitialized bytes using Valgrind
  for (i = 0; i < off; i++)
  {
    printf("i %d ", (int)i);
    printf("bufi %d\n", (uint8_t)buf[i]);
  }
#endif
  for (i = 0; i <= off/* - dnsstrlen*/; i++)
  {
    char *dnsbuf = dns;
    size_t j = i;
    //printf("Comparing %zu: %s\n", i, dns);
    for (;;)
    {
      char *chr = strchr(dnsbuf, '.');
      if (chr == NULL)
      {
        chr = dnsbuf+strlen(dnsbuf);
      }
      if (j >= off)
      {
        break;
      }
#if 0
      printf("j %d ", (int)j);
      printf("bufj %d ", (uint8_t)buf[j]);
      printf("off %zu\n", off);
#endif
      if ((buf[j] & 0xC0) == 0xC0)
      {
        size_t newj = 0;
        newj += ((size_t)(buf[j++] & 0x3F)<<8);
        if (j >= off)
        {
          break;
        }
        newj += ((size_t)(buf[j++] & 0xFF)<<0);
        j = newj;
      }
      u8 = ((uint8_t)(chr-dnsbuf));
#if 1
      if (j >= off)
      {
        break;
      }
#endif
      if ((uint8_t)buf[j++] != u8)
      {
        //printf("Comparing %zu ended at %zu u8 %d buf %d\n", i, j, u8, buf[j-1]);
        break;
      }
      if (chr == dnsbuf)
      {
        if ((ssize_t)i < 0)
        {
          return -1;
        }
        //printf("found match %zu\n", i);
        return (ssize_t)i;
      }
      if (j + u8 >= off)
      {
        break;
      }
      if (memcmp(&buf[j], dnsbuf, u8) != 0)
      {
        //printf("Comparing %zu ended at %zu: %s\n", i, j, dnsbuf);
        break;
      }
      j += u8;
      if (*chr == '\0')
      {
        dnsbuf = chr;
      }
      else
      {
        dnsbuf = chr+1;
      }
    }
  }
  return -1;
}

int dns_put_next(void *vdns, uint16_t *off, uint16_t *remcnt,
                 uint16_t plen,
                 char *buf,
                 uint16_t qtype, uint16_t qclass, uint32_t ttl,
                 uint16_t rdlength, void *rdata)
{
  unsigned char *cdns = vdns;
  char *chr;
  ssize_t matchoff;
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
    matchoff = memdns(vdns, *off, buf);
    if (chr != buf && matchoff >= 0 && matchoff < 0x4000 && (*off) + 2 <= plen)
    {
      cdns[(*off)++] = 0xc0 | (matchoff>>8);
      cdns[(*off)++] = matchoff;
      break;
    }
    if (*off + 1 + (chr-buf) > plen)
    {
      return -EFAULT;
    }
    cdns[(*off)++] = chr-buf;
    memcpy(&cdns[*off], buf, (size_t)(chr-buf));
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
  ssize_t matchoff;
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
    matchoff = memdns(vdns, *off, buf);
    if (chr != buf && matchoff >= 0 && matchoff < 0x4000 && (*off) + 2 <= plen)
    {
      cdns[(*off)++] = 0xc0 | (matchoff>>8);
      cdns[(*off)++] = matchoff;
      break;
    }
    if (*off + 1 + (chr-buf) > plen)
    {
      return -EFAULT;
    }
    cdns[(*off)++] = chr-buf;
    memcpy(&cdns[*off], buf, (size_t)(chr-buf));
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

int dns_next(const void *vdns, uint16_t *off, uint16_t *remcnt,
             uint16_t plen,
             char *buf, size_t bufsiz, uint16_t *qtype,
             uint16_t *qclass)
{
  const unsigned char *cdns = vdns;
  uint16_t tocopy;
  uint16_t labsiz, laboff;
  size_t strlentmp;
  int jmp = 0;
  *buf = '\0';
  if (*remcnt == 0)
  {
    return -ENOENT;
  }
  laboff = *off;
  while (*off < plen && *remcnt)
  {
    labsiz = cdns[laboff++];
    //printf("labsiz %d at off %d\n", (int)labsiz, (int)laboff);
    if (!jmp)
    {
      (*off)++;
    }
    if (labsiz == 0)
    {
      break;
    }
    if ((labsiz & 0xc0) == 0xc0)
    {
      labsiz &= ~0xc0;
      labsiz <<= 8;
      if (laboff >= plen)
      {
        return -EFAULT;
      }
      labsiz |= cdns[laboff++];
      if (!jmp)
      {
        (*off)++;
      }
      if (labsiz >= plen)
      {
        return -EFAULT;
      }
      laboff = labsiz + 1;
      labsiz = cdns[laboff - 1];
      jmp = 1;
    }
    else
    {
      //laboff = *off;
      if (!jmp)
      {
        (*off) += labsiz;
      }
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
    memcpy(buf+strlentmp, &cdns[laboff], tocopy);
    memcpy(buf+strlentmp+tocopy, ".\0", 2);
    //printf("buf now %s\n", buf);
    strlentmp += (size_t)tocopy + 1;
    laboff += labsiz;
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

int recursive_resolve(const char *pkt, size_t recvd, const char *name,
                      uint16_t qclassexp, uint16_t *qtypeptr,
                      char *databuf, size_t databufsiz, size_t *datalen)
{
  uint16_t aoff, answer_aoff;
  uint16_t remcnt;
  int i;
  uint32_t ttl;
  const int recursion_limit = 100;
  char nambuf[8192];
  char nextbuf[8192];
  uint16_t qtype, qclass;
  size_t vallen;
  dns_next_init_qd(pkt, &aoff, &remcnt, recvd);
  if (dns_next(pkt, &aoff, &remcnt, recvd, nambuf, sizeof(nambuf), &qtype, &qclass) == 0)
  {
    if (strcmp(nambuf, name) != 0)
    {
      return -ENOENT;
    }
  }
  else
  {
    return -ENOENT;
  }
  snprintf(nextbuf, sizeof(nextbuf), "%s.", name);
  if (dns_next(pkt, &aoff, &remcnt, recvd, nambuf, sizeof(nambuf), &qtype, &qclass) == 0)
  {
    return -EFAULT;
  }
  answer_aoff = aoff;
  dns_next_init_an_ask(pkt, &aoff, &remcnt, recvd);
  for (i = 0; i < recursion_limit; i++)
  {
    aoff = answer_aoff;
    dns_next_init_an_ask(pkt, &aoff, &remcnt, recvd);
    for (;;)
    {
      vallen = 0;
      if (dns_next_an_ask(pkt, &aoff, &remcnt, recvd, nambuf, sizeof(nambuf), &qtype, &qclass, &ttl,
                          databuf, databufsiz, &vallen) != 0)
      {
        return -ENOENT;
      }
      if (strcmp(nambuf, nextbuf) != 0)
      {
        continue;
      }
      if (qclass != qclassexp)
      {
        return -ENOENT;
      }
      if (qtype == 5)
      {
        snprintf(nextbuf, sizeof(nextbuf), "%s.", databuf);
        //printf("got cname %s\n", nextbuf);
        break;
      }
      if (qtype != 5)
      {
        *qtypeptr = qtype;
        *datalen = vallen;
        return 0;
      }
    }
  }
  return -ENOENT;
}
