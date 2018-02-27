#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include "byteswap.h"
#include "hdr.h"
#include "iphdr.h"
#include "mypcapjoker.h"
#include "mypcapng.h"

static void usage(const char *argv0)
{
  fprintf(stderr, "Usage: %s [-i eth0] [-o eth1] [-a 1.2.3.4] in.pcapng out.pcap\n", argv0);
  exit(1);
}

int main(int argc, char **argv)
{
  struct pcap_joker_ctx inctx;
  struct pcap_out_ctx outctx;
  void *buf = NULL;
  size_t bufcapacity = 0;
  size_t len, snap;
  int result;
  uint64_t time64;
  int opt;
  int truncated = 0;
  while ((opt = getopt(argc, argv, "")) != -1)
  {
    switch (opt)
    {
      default:
        usage(argv[0]);
    }
  }
  if (optind + 2 != argc)
  {
    usage(argv[0]);
  }
  if (pcap_joker_ctx_init(&inctx, argv[optind], 1, NULL) != 0)
  {
    printf("can't open input file\n");
    exit(1);
  }
  if (pcap_out_ctx_init(&outctx, argv[optind + 1]) != 0)
  {
    printf("can't open output file\n");
    exit(1);
  }
  while ((result = pcap_joker_ctx_read(&inctx, &buf, &bufcapacity, &len, &snap, &time64, NULL)) > 0)
  {
    if (snap != len && !truncated)
    {
      printf("snap %zu len %zu\n", snap, len);
      printf("warning: truncated packets not fully supported\n");
      truncated = 1;
    }
    if (pcap_out_ctx_write(&outctx, buf, snap, time64) != 0)
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
  pcap_joker_ctx_free(&inctx);
  pcap_out_ctx_free(&outctx);
  free(buf);
  return 0;
}
