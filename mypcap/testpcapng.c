#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "byteswap.h"
#include "hdr.h"
#include "mypcap.h"
#include "mypcapng.h"

int main(int argc, char **argv)
{
  struct pcapng_in_ctx inctx;
  struct pcapng_out_ctx outctx;
  void *buf = NULL;
  size_t bufcapacity = 0;
  size_t len, snap;
  int result;
  uint64_t time64;
  const char *ifname;
  if (pcapng_in_ctx_init(&inctx, "pcap.pcapng", 1) != 0)
  {
    printf("can't init ctx\n");
    abort();
  }
  if (pcapng_out_ctx_init(&outctx, "pcap2.pcapng") != 0)
  {
    printf("can't init outctx\n");
    abort();
  }
  while ((result = pcapng_in_ctx_read(&inctx, &buf, &bufcapacity, &len, &snap, &time64, &ifname)) > 0)
  {
    time_t time = (time_t)(time64/1000/1000);
    printf("ifname %s len %zu snap %zu time %s\n", ifname, len, snap, ctime(&time));
    if (pcapng_out_ctx_write(&outctx, buf, snap, time64, "eth0") != 0)
    {
      printf("write erron\n");
      abort();
    }
  }
  if (result < 0)
  {
    printf("error\n");
    abort();
  }
  pcapng_in_ctx_free(&inctx);
  pcapng_out_ctx_free(&outctx);
  free(buf);
  return 0;
}
