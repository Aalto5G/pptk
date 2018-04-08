#ifndef _LINKCOMMON_H_
#define _LINKCOMMON_H_

#include <errno.h>
#include "ldp.h"

int ldp_set_promisc_mode(int sockfd, const char *ifname, int on);

static inline int
ldp_interface_set_promisc_mode(struct ldp_interface *intf, int on)
{
  int ret, sockfd;
  if (intf->promisc_mode_set)
  {
    return intf->promisc_mode_set(intf, on);
  }
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    return -1;
  }
  ret = ldp_set_promisc_mode(sockfd, intf->name, on);
  close(sockfd);
  return ret;
}

int ldp_link_status(int sockfd, const char *ifname);

static inline int ldp_interface_link_status(struct ldp_interface *intf)
{
  int ret, sockfd;
  if (intf->link_status)
  {
    return intf->link_status(intf);
  }
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    return 0;
  }
  ret = ldp_link_status(sockfd, intf->name);
  close(sockfd);
  return ret;
}

int ldp_link_wait(int sockfd, const char *ifname);

static inline int ldp_interface_link_wait(struct ldp_interface *intf)
{
  int ret, sockfd;
  if (intf->link_wait)
  {
    return intf->link_wait(intf);
  }
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    return -ENETDOWN;
  }
  ret = ldp_link_wait(sockfd, intf->name);
  close(sockfd);
  return ret;
}

int ldp_mac_addr(int sockfd, const char *ifname, void *mac);

static inline int ldp_interface_mac_addr(struct ldp_interface *intf, void *mac)
{
  int ret, sockfd;
  if (intf->mac_addr)
  {
    return intf->mac_addr(intf, mac);
  }
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    if (errno > 0)
    {
      return -errno;
    }
    else
    {
      return -ENETDOWN;
    }
  }
  ret = ldp_mac_addr(sockfd, intf->name, mac);
  close(sockfd);
  return ret;
}

#endif
