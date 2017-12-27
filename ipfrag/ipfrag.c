#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "allocif.h"
#include "iphdr.h"
#include "packet.h"
#include "ipcksum.h"
#include "mypcap.h"
#include "ipfrag.h"

int fragment4(struct allocif *loc, const void *pkt, uint16_t sz,
              struct fragment *frags, size_t fragnum)
{
  const void *ether = pkt;
  const void *ip = ether_const_payload(ether);
  const char *cpay;
  uint8_t iphdrlen;
  uint16_t datamax;
  size_t i;
  if (ether_type(ether) != ETHER_TYPE_IP)
  {
    printf("%x\n", (uint32_t)(uint16_t)ether_type(ether));
    printf("1\n");
    abort();
  }
  if (ip_version(ip) != 4)
  {
    printf("2\n");
    abort();
  }
  iphdrlen = ip_hdr_len(ip);
  if (iphdrlen < IP_HDR_MINLEN)
  {
    printf("3\n");
    abort();
  }
  if (ETHER_HDR_LEN + iphdrlen > sz)
  {
    printf("4\n");
    abort();
  }
  if (ip_dont_frag(ip))
  {
    printf("5\n");
    abort();
  }
  if (ip_more_frags(ip))
  {
    printf("6\n");
    abort();
  }
  if (ip_frag_off(ip))
  {
    printf("7\n");
    abort();
  }
  if (ip_hdr_cksum_calc(ip, iphdrlen) != 0)
  {
    printf("8\n");
    abort();
  }
  cpay = ip_const_payload(ip);
  datamax = sz - ETHER_HDR_LEN - iphdrlen;
  for (i = 0; i < fragnum; i++)
  {
    if (frags[i].datastart % 8 != 0)
    {
      printf("9\n");
      abort();
    }
    if (frags[i].datastart > datamax)
    {
      printf("10\n");
      abort();
    }
    if (frags[i].datastart + frags[i].datalen > datamax)
    {
      printf("11 i=%zu\n",i );
      abort();
    }
    if (frags[i].pkt != NULL)
    {
      printf("12\n");
      abort();
    }
  }
  for (i = 0; i < fragnum; i++)
  {
    frags[i].pkt =
      allocif_alloc(loc,
                    packet_size(ETHER_HDR_LEN + iphdrlen + frags[i].datalen));
    if (frags[i].pkt == NULL)
    {
      for (;;)
      {
        if (i == 0)
        {
          return -ENOMEM;
        }
        i--;
        allocif_free(loc, frags[i].pkt);
      }
    }
  }
  for (i = 0; i < fragnum; i++)
  {
    void *ether2;
    void *ip2;
    void *pay2;
    ether2 = packet_data(frags[i].pkt);
    frags[i].pkt->sz = ETHER_HDR_LEN + iphdrlen + frags[i].datalen;
    memcpy(ether2, ether, ETHER_HDR_LEN + iphdrlen);
    ip2 = ether_payload(ether2);
    ip_set_frag_off(ip2, frags[i].datastart);
    ip_set_more_frags(ip2, frags[i].datastart + frags[i].datalen < datamax);
    ip_set_total_len(ip2, iphdrlen + frags[i].datalen);
    ip_set_hdr_cksum_calc(ip2, iphdrlen);
    pay2 = ip_payload(ip2);
    memcpy(pay2, cpay + frags[i].datastart, frags[i].datalen);
  }
  return 0;
}
