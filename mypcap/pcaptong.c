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

static void usage(const char *argv0)
{
  fprintf(stderr, "Usage: %s [-i eth0] in.pcap out.pcapng\n", argv0);
  exit(1);
}

int main(int argc, char **argv)
{
  struct pcap_in_ctx inctx;
  struct pcapng_out_ctx outctx;
  void *buf = NULL;
  size_t bufcapacity = 0;
  size_t len, snap;
  int result;
  uint64_t time64;
  const char *ifname = "eth0";
  int opt;
  int truncated = 0;
  while ((opt = getopt(argc, argv, "i:")) != -1)
  {
    switch (opt)
    {
      case 'i':
        ifname = optarg;
        break;
      default:
        usage(argv[0]);
    }
  }
  if (optind + 2 != argc)
  {
    usage(argv[0]);
  }
  if (pcap_in_ctx_init(&inctx, argv[optind], 1) != 0)
  {
    printf("can't open input file\n");
    exit(1);
  }
  if (pcapng_out_ctx_init(&outctx, argv[optind + 1]) != 0)
  {
    printf("can't open output file\n");
    exit(1);
  }
  while ((result = pcap_in_ctx_read(&inctx, &buf, &bufcapacity, &len, &snap, &time64)) > 0)
  {
    if (snap != len && !truncated)
    {
      printf("warning: truncated packets not fully supported\n");
      truncated = 1;
    }
    if (pcapng_out_ctx_write(&outctx, buf, snap, time64, ifname) != 0)
    {
      printf("write error\n");
      exit(1);
    }
  }
  if (result < 0)
  {
    printf("read error\n");
    exit(1);
  }
  pcap_in_ctx_free(&inctx);
  pcapng_out_ctx_free(&outctx);
  free(buf);
  return 0;
}
