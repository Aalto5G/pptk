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
    printf("usage: %s tap0\n", argv[0]);
    exit(1);
  }
  fd = tap_alloc(argv[1], actual_dev);
  if (fd < 0)
  {
    printf("failed to open tap\n");
    exit(1);
  }
  if (strcmp(actual_dev, argv[1]) != 0)
  {
    abort();
  }
  if (tap_bring_up(actual_dev) != 0)
  {
    abort();
  }
  for (;;)
  {
    pause();
  }
  return 0;
}
