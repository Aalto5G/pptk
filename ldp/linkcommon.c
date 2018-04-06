#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include "linkcommon.h"

int ldp_set_promisc_mode(int sockfd, const char *ifname, int on)
{
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  if (strncmp(ifname, "null:", 5) == 0)
  {
    return 0;
  }
  if (strncmp(ifname, "vale", 4) == 0)
  {
    return 0;
  }
  if (strncmp(ifname, "netmap:", 7) == 0)
  {
    ifname = ifname + 7;
  }
  if (on)
  {
    snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    ifr.ifr_flags &= ~IFF_PROMISC;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    ifr.ifr_flags |= IFF_PROMISC;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
  }
  else
  {
    snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    ifr.ifr_flags |= IFF_PROMISC;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    ifr.ifr_flags &= ~IFF_PROMISC;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
  }
  return 0;
}

int ldp_link_status(int sockfd, const char *ifname)
{
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  if (strncmp(ifname, "vale", 4) == 0)
  {
    return 1;
  }
  if (strncmp(ifname, "null:", 5) == 0)
  {
    return 1;
  }
  if (strncmp(ifname, "netmap:", 7) == 0)
  {
    ifname = ifname + 7;
  }
  snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
  if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
  {
    return 0;
  }
  return !!(ifr.ifr_flags & IFF_RUNNING);
}

int ldp_link_wait(int sockfd, const char *ifname)
{
  int i;
  for (i = 0; i <= 10; i++)
  {
    if (ldp_link_status(sockfd, ifname))
    {
      sleep(1); // Better safe than sorry
      return 0;
    }
    sleep(1);
  }
  return -ENETDOWN;
}
