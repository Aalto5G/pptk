#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "byteswap.h"
#include "hdr.h"
#include "mypcap.h"
#include "mypcapng.h"
#include "iphdr.h"

static void usage(const char *argv0)
{
  fprintf(stderr, "Usage: %s [-i] in1.pcapng in2.pcapng\n", argv0);
  exit(1);
}

int main(int argc, char **argv)
{
  struct pcapng_in_ctx inctx1;
  struct pcapng_in_ctx inctx2;
  void *buf1 = NULL, *buf2 = NULL;
  const char *ifname1, *ifname2;
  size_t buf1capacity = 0, buf2capacity = 0;
  size_t len1, snap1;
  size_t len2, snap2;
  int result1, result2;
  int opt;
  int no_ipid = 0;
  while ((opt = getopt(argc, argv, "i")) != -1)
  {
    switch (opt)
    {
      case 'i':
        no_ipid = 1;
        break;
      default:
        usage(argv[0]);
    }
  }
  if (optind + 2 != argc)
  {
    usage(argv[0]);
  }
  if (pcapng_in_ctx_init(&inctx1, argv[optind], 1) != 0)
  {
    printf("can't open first input file\n");
    exit(1);
  }
  if (pcapng_in_ctx_init(&inctx2, argv[optind + 1], 1) != 0)
  {
    printf("can't open second input file\n");
    exit(1);
  }
  for (;;)
  {
    result1 = pcapng_in_ctx_read(&inctx1, &buf1, &buf1capacity, &len1, &snap1, NULL, &ifname1);
    result2 = pcapng_in_ctx_read(&inctx2, &buf2, &buf2capacity, &len2, &snap2, NULL, &ifname2);
    if (result1 < 0)
    {
      printf("error reading from first input file\n");
      exit(1);
    }
    if (result2 < 0)
    {
      printf("error reading from second input file\n");
      exit(1);
    }
    if (result1 == 0 && result2 == 0)
    {
      break;
    }
    if (result1 != result2)
    {
      printf("file lengths differ\n");
      exit(1);
    }
    if (snap1 != snap2)
    {
      printf("packet snaps differ\n");
      exit(1);
    }
    if (len1 != len2)
    {
      printf("packet lengths differ\n");
      exit(1);
    }
    if ((ifname1 == NULL) != (ifname2 == NULL))
    {
      printf("ifnames differ\n");
      exit(1);
    }
    if (ifname1 != NULL && ifname2 != NULL && strcmp(ifname1, ifname2) != 0)
    {
      printf("ifnames differ\n");
      exit(1);
    }
    if (snap1 < 14+6)
    {
      if (memcmp(buf1, buf2, snap1) != 0)
      {
        printf("packet datas differ\n");
        exit(1);
      }
    }
    if (ether_type(buf1) == 0x0800 && ether_type(buf2) == 0x0800 && no_ipid)
    {
      if (memcmp(buf1, buf2, 14+4) != 0)
      {
        printf("packet datas differ\n");
        exit(1);
      }
      if (memcmp(((char*)buf1)+14+6, ((char*)buf2)+14+6, snap1-14-6) != 0)
      {
        printf("packet datas differ\n");
        exit(1);
      }
    }
    else if (memcmp(buf1, buf2, snap1) != 0)
    {
      printf("packet datas differ\n");
      exit(1);
    }
  }
  pcapng_in_ctx_free(&inctx1);
  pcapng_in_ctx_free(&inctx2);
  free(buf1);
  free(buf2);
  return 0;
}
