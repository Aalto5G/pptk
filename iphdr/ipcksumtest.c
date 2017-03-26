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
  char iptcphdr[] = "\x45\x0\x0\x2b\x0\x1\x0\x0\x40\x6\x7c\xca\x7f\x0\x0\x1\x7f\x0\x0\x1\x0\x14\x0\x50\x0\x0\x0\x0\x0\x0\x0\x0\x50\x2\x20\x0\xbc\x9\x0\x0\x66\x6f\x6f";
  struct ip_cksum_ctx ctx = IP_CKSUM_CTX_INITER;
  struct ip_cksum_ctx ctx2 = IP_CKSUM_CTX_INITER;
  struct ip_cksum_ctx ctx3 = IP_CKSUM_CTX_INITER;
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

  if (ip_hdr_cksum_calc(iphdr, sizeof(iphdr)-1) != 0)
  {
    abort();
  }
  if (tcp_cksum_calc(iptcphdr, 20, iptcphdr+20, sizeof(iptcphdr)-20-1) != 0)
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

  return 0;
}
