#include "tuntap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  char actual_dev[IFNAMSIZ];
  int fd;
  if (argc != 2)
  {
    printf("usage: %s tun0\n", argv[0]);
    exit(1);
  }
  fd = tun_alloc(argv[1], actual_dev);
  if (fd < 0)
  {
    printf("failed to open tun\n");
    exit(1);
  }
  if (strcmp(actual_dev, argv[1]) != 0)
  {
    abort();
  }
  if (tun_bring_up_addr(actual_dev, (10<<24)|(111<<16)|1, (10<<24)|(111<<16)|2) != 0)
  {
    abort();
  }
  for (;;)
  {
    pause();
  }
  return 0;
}
