#include "ipcksum.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "hdr.h"
#include "iphdr.h"

uint16_t ip_hdr_cksum_calc(const void *iphdr, uint16_t iplen)
{
  uint8_t ihl = ip_hdr_len(iphdr);
  struct ip_cksum_ctx ctx = IP_CKSUM_CTX_INITER;
  if (ihl > iplen)
  {
    abort();
  }
  ip_cksum_feed(&ctx, iphdr, ihl);
  return ip_cksum_postprocess(&ctx);
}

uint16_t tcp_hdr_cksum_calc(
  const void *iphdr, uint16_t iplen, const void *tcphdr, uint16_t tcplen)
{
  struct ip_cksum_ctx ctx = IP_CKSUM_CTX_INITER;
  uint32_t u32;
  if (iplen < 20)
  {
    abort();
  }
  u32 = ip_src(iphdr);
  ip_cksum_feed32ptr(&ctx, &u32);
  u32 = ip_dst(iphdr);
  ip_cksum_feed32ptr(&ctx, &u32);
  ip_cksum_add16(&ctx, htons(6)); // FIXME verify
  ip_cksum_add16(&ctx, tcplen);
  ip_cksum_feed(&ctx, tcphdr, tcplen);
  return ip_cksum_postprocess(&ctx);
}
