#include "tuntap.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>

int tap_alloc(const char *devreq, char actual_dev[IFNAMSIZ])
{
  struct ifreq ifr;
  int fd, err;

  fd = open("/dev/net/tun", O_RDWR);
  if (fd < 0)
  {
    return -1;
  }

  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = IFF_TAP; 
  if (devreq && *devreq)
  {
    strncpy(ifr.ifr_name, devreq, IFNAMSIZ-1);
  }

  err = ioctl(fd, TUNSETIFF, (void *)&ifr);
  if (err < 0) 
  {
     close(fd);
     return err;
  }
  snprintf(actual_dev, IFNAMSIZ, "%s", ifr.ifr_name);
  return fd;
}

int tun_alloc(const char *devreq, char actual_dev[IFNAMSIZ])
{
  struct ifreq ifr;
  int fd, err;

  fd = open("/dev/net/tun", O_RDWR);
  if (fd < 0)
  {
    return -1;
  }

  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = IFF_TUN; 
  if (devreq && *devreq)
  {
    strncpy(ifr.ifr_name, devreq, IFNAMSIZ-1);
  }

  err = ioctl(fd, TUNSETIFF, (void *)&ifr);
  if (err < 0) 
  {
     close(fd);
     return err;
  }
  snprintf(actual_dev, IFNAMSIZ, "%s", ifr.ifr_name);
  return fd;
}

static int safe_exec(char **buf)
{
  pid_t x;
  x = fork();
  if (x < 0)
  {
    return -1;
  }
  else if (x == 0)
  {
    execv(buf[0], buf);
    exit(1);
  }
  else
  {
    int status;
    if (waitpid(x, &status, 0) != x)
    {
      return -1;
    }
    if (WIFEXITED(status))
    {
      return WEXITSTATUS(status);
    }
    return -1;
  }
}

int tap_bring_up(const char *actual_dev)
{
  int x = -1;
  int ipaddr_last = 0;
  char addrbuf[64] = {0};
  char devbuf[64] = {0};
  char *cmdbuf[] = {
    "/sbin/ip",
    "addr",
    "add",
    addrbuf,
    "dev",
    devbuf,
    NULL
  };
  char *cmd2buf[] = {
    "/sbin/ip",
    "link",
    "set",
    devbuf,
    "up",
    NULL
  };
  int status;

  snprintf(devbuf, sizeof(devbuf), "%s", actual_dev);
  if (sscanf(actual_dev, "tap%d", &x) != 1)
  {
    return -1;
  }
  ipaddr_last = x+1;
  snprintf(addrbuf, sizeof(addrbuf), "0.0.0.%d/32", ipaddr_last);
  status = safe_exec(cmdbuf);
  if (status)
  {
    return status;
  }
  return safe_exec(cmd2buf);
}

int tun_bring_up_addr(const char *actual_dev, uint32_t addr, uint32_t peer)
{
  int x = -1;
  char addrbuf[64] = {0};
  char addr2buf[64] = {0};
  char devbuf[64] = {0};
  char *cmdbuf[] = {
    "/sbin/ip",
    "addr",
    "add",
    addrbuf,
    "peer",
    addr2buf,
    "dev",
    devbuf,
    NULL
  };
  char *cmd2buf[] = {
    "/sbin/ip",
    "link",
    "set",
    devbuf,
    "up",
    NULL
  };
  int status;

  snprintf(devbuf, sizeof(devbuf), "%s", actual_dev);
  if (sscanf(actual_dev, "tun%d", &x) != 1)
  {
    return -1;
  }
  snprintf(addrbuf, sizeof(addrbuf), "%d.%d.%d.%d",
           (addr>>24)&0xff, (addr>>16)&0xff,
           (addr>>8)&0xff, (addr>>0)&0xff);
  snprintf(addr2buf, sizeof(addr2buf), "%d.%d.%d.%d",
           (peer>>24)&0xff, (peer>>16)&0xff,
           (peer>>8)&0xff, (peer>>0)&0xff);
  status = safe_exec(cmdbuf);
  if (status)
  {
    return status;
  }
  return safe_exec(cmd2buf);
}
