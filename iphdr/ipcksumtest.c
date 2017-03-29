#include "ipcksum.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "hdr.h"
#include "iphdr.h"

int main(int argc, char **argv)
{
  char *buf = "abcdef"; 
  char *buf2 = "abcdefg"; 
  char *buf3 = "abcdefghijklmnopqrstuvwxyz";
  char iphdr[] = "\x45\x0\x0\x14\x0\x1\x0\x0\x40\x0\x7c\xe7\x7f\x0\x0\x1\x7f\x0\x0\x1";
  char iptcphdr[] = "\x45\x00\x00\x2b\x00\x01\x00\x00\x40\x06\x7c\xca\x7f\x00\x00\x01\x7f\x00\x00\x01\x00\x14\x00\x50\x00\x00\x00\x00\x00\x00\x00\x00\x50\x02\x20\x00\xbc\x09\x00\x00\x66\x6f\x6f";
  char ipudphdr[] = "\x45\x0\x0\x1f\x0\x1\x0\x0\x40\x11\x7c\xcb\x7f\x0\x0\x1\x7f\x0\x0\x1\x0\x35\x0\x35\x0\xb\x2b\xfc\x66\x6f\x6f";
  struct ip_cksum_ctx ctx = IP_CKSUM_CTX_INITER;
  struct ip_cksum_ctx ctx2 = IP_CKSUM_CTX_INITER;
  struct ip_cksum_ctx ctx3 = IP_CKSUM_CTX_INITER;
  int i;
  ip_cksum_feed(&ctx, buf, strlen(buf));
  if (ip_cksum_postprocess(&ctx) != 54738)
  {
    abort();
  }
  ip_cksum_feed(&ctx2, buf2, strlen(buf2));
  if (ip_cksum_postprocess(&ctx2) != 28370)
  {
    abort();
  }
  ip_cksum_feed(&ctx3, buf3, strlen(buf3));
  if (ip_cksum_postprocess(&ctx3) != 29028)
  {
    abort();
  }

  for (i = 0; i < 1000000; i++)
  {
    uint32_t new_src = rand();
    uint32_t new_dst = rand();
    uint16_t new_src_port = rand();
    uint16_t new_dst_port = rand();
    ip_set_src_cksum_update(iptcphdr, 20, 6, iptcphdr+20, sizeof(iptcphdr)-20-1, new_src);
    ip_set_dst_cksum_update(iptcphdr, 20, 6, iptcphdr+20, sizeof(iptcphdr)-20-1, new_dst);
    tcp_set_src_port_cksum_update(iptcphdr+20, sizeof(iptcphdr)-20-1, new_src_port);
    tcp_set_dst_port_cksum_update(iptcphdr+20, sizeof(iptcphdr)-20-1, new_dst_port);
    if (ip_hdr_cksum_calc(iptcphdr, 20) != 0)
    {
      abort();
    }
    if (tcp_cksum_calc(iptcphdr, 20, iptcphdr+20, sizeof(iptcphdr)-20-1) != 0)
    {
      abort();
    }
  }
  ip_decr_ttl_cksum_update(iptcphdr);
  if (ip_hdr_cksum_calc(iptcphdr, 20) != 0)
  {
    abort();
  }
  if (tcp_cksum_calc(iptcphdr, 20, iptcphdr+20, sizeof(iptcphdr)-20-1) != 0)
  {
    abort();
  }

  if (ip_hdr_cksum_calc(iphdr, sizeof(iphdr)-1) != 0)
  {
    abort();
  }
  if (tcp_cksum_calc(iptcphdr, 20, iptcphdr+20, sizeof(iptcphdr)-20-1) != 0)
  {
    abort();
  }
  if (udp_cksum_calc(ipudphdr, 20, ipudphdr+20, sizeof(ipudphdr)-20-1) != 0)
  {
    abort();
  }

  ip_set_hdr_cksum_calc(iphdr, sizeof(iphdr)-1);
  if (ip_hdr_cksum_calc(iphdr, sizeof(iphdr)-1) != 0)
  {
    abort();
  }
  tcp_set_cksum_calc(iptcphdr, 20, iptcphdr+20, sizeof(iptcphdr)-20-1);
  if (tcp_cksum_calc(iptcphdr, 20, iptcphdr+20, sizeof(iptcphdr)-20-1) != 0)
  {
    abort();
  }
  udp_set_cksum_calc(ipudphdr, 20, ipudphdr+20, sizeof(ipudphdr)-20-1);
  if (udp_cksum_calc(ipudphdr, 20, ipudphdr+20, sizeof(ipudphdr)-20-1) != 0)
  {
    abort();
  }

  return 0;
}
