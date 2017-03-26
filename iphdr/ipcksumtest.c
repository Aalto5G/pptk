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
  return 0;
}
