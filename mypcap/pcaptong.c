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
  fprintf(stderr, "Usage: %s [-i eth0] [-o eth1] [-a 1.2.3.4] in.pcap out.pcapng\n", argv0);
  exit(1);
}

int main(int argc, char **argv)
{
  struct pcap_joker_ctx inctx;
  struct pcapng_out_ctx outctx;
  void *buf = NULL;
  size_t bufcapacity = 0;
  size_t len, snap;
  int result;
  uint64_t time64;
  const char *ifname = "eth0";
  const char *outifname = NULL;
  uint32_t outaddr = 0;
  int opt;
  int truncated = 0;
  while ((opt = getopt(argc, argv, "i:o:a:")) != -1)
  {
    switch (opt)
    {
      case 'i':
        ifname = optarg;
        break;
      case 'o':
        outifname = optarg;
        break;
      case 'a':
        outaddr = inet_addr(optarg);
        if (outaddr == INADDR_NONE)
        {
          printf("invalid address %s\n", optarg);
          usage(argv[0]);
        }
        break;
      default:
        usage(argv[0]);
    }
  }
  if (outifname == NULL && outaddr != 0)
  {
    printf("with -a you have to specify -o\n");
    usage(argv[0]);
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
  if (pcapng_out_ctx_init(&outctx, argv[optind + 1]) != 0)
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
    if (snap >= 14+20)
    {
      void *ip = ether_payload(buf);
      if (outaddr && ip_src(ip) == ntohl(outaddr))
      {
        if (pcapng_out_ctx_write(&outctx, buf, snap, time64, outifname) != 0)
        {
          printf("write error\n");
          exit(1);
        }
        continue;
      }
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
  pcap_joker_ctx_free(&inctx);
  pcapng_out_ctx_free(&outctx);
  free(buf);
  return 0;
}
