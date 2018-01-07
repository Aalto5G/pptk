#define NETMAP_WITH_LIBS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include "net/netmap_user.h"
#include "netmapcommon.h"
#include "log.h"

void set_promisc_mode(int sockfd, const char *ifname, int on)
{
  struct ifreq ifr;
  if (strncmp(ifname, "vale", 4) == 0)
  {
    return;
  }
  if (strncmp(ifname, "netmap:", 7) != 0)
  {
    log_log(LOG_LEVEL_CRIT, "NETMAPCOMMON",
            "invalid interface name, does not begin with netmap:");
    exit(1);
  }
  if (on)
  {
    snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname + 7);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
    {
      log_log(LOG_LEVEL_CRIT, "NETMAPCOMMON",
              "can't get interface flags");
      exit(1);
    }
    ifr.ifr_flags &= ~IFF_PROMISC;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
    {
      log_log(LOG_LEVEL_CRIT, "NETMAPCOMMON",
              "can't turn promiscuous mode off");
      exit(1);
    }
    snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname + 7);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
    {
      log_log(LOG_LEVEL_CRIT, "NETMAPCOMMON",
              "can't get interface flags");
      exit(1);
    }
    ifr.ifr_flags |= IFF_PROMISC;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
    {
      log_log(LOG_LEVEL_CRIT, "NETMAPCOMMON",
              "can't turn promiscuous mode on");
      exit(1);
    }
  }
  else
  {
    snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname + 7);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
    {
      log_log(LOG_LEVEL_CRIT, "NETMAPCOMMON",
              "can't get interface flags");
      exit(1);
    }
    ifr.ifr_flags |= IFF_PROMISC;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
    {
      log_log(LOG_LEVEL_CRIT, "NETMAPCOMMON",
              "can't turn promiscuous mode on");
      exit(1);
    }
    snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname + 7);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
    {
      log_log(LOG_LEVEL_CRIT, "NETMAPCOMMON",
              "can't get interface flags");
      exit(1);
    }
    ifr.ifr_flags &= ~IFF_PROMISC;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
    {
      log_log(LOG_LEVEL_CRIT, "NETMAPCOMMON",
              "can't turn promiscuous mode off");
      exit(1);
    }
  }
}

int link_status(int sockfd, const char *ifname)
{
  struct ifreq ifr;
  if (strncmp(ifname, "vale", 4) == 0)
  {
    return 1;
  }
  if (strncmp(ifname, "netmap:", 7) != 0)
  {
    log_log(LOG_LEVEL_CRIT, "NETMAPCOMMON",
            "invalid interface name, does not begin with netmap:");
    exit(1);
  }
  snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname + 7);
  if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
  {
    log_log(LOG_LEVEL_CRIT, "NETMAPCOMMON",
            "can't get interface flags");
    exit(1);
  }
  return !!(ifr.ifr_flags & IFF_RUNNING);
}

void link_wait(int sockfd, const char *ifname)
{
  int i;
  for (i = 0; i <= 10; i++)
  {
    if (link_status(sockfd, ifname))
    {
      sleep(1); // Better safe than sorry
      return;
    }
    sleep(1);
  }
  log_log(LOG_LEVEL_CRIT, "NETMAPCOMMON",
          "interface %s is down", ifname);
  exit(1);
}
