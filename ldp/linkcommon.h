#ifndef _LINKCOMMON_H_
#define _LINKCOMMON_H_

#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include "ldp.h"

int ldp_set_mtu(int sockfd, const char *ifname, uint16_t mtu);

static inline int
ldp_interface_set_mtu(struct ldp_interface *intf, uint16_t mtu)
{
  int ret, sockfd;
#if 0
  if (intf->mtu_set)
  {
    return intf->mtu_set(intf, on);
  }
#endif
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    return -1;
  }
  ret = ldp_set_mtu(sockfd, intf->name, mtu);
  close(sockfd);
  return ret;
}

int ldp_set_promisc_mode(int sockfd, const char *ifname, int on);

int ldp_get_promisc_mode(int sockfd, const char *ifname);

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

static inline int
ldp_interface_get_promisc_mode(struct ldp_interface *intf)
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
  ret = ldp_get_promisc_mode(sockfd, intf->name);
  close(sockfd);
  return ret;
}

int ldp_set_allmulti(int sockfd, const char *ifname, int on);

static inline int
ldp_interface_set_allmulti(struct ldp_interface *intf, int on)
{
  int ret, sockfd;
  if (intf->allmulti_set)
  {
    return intf->allmulti_set(intf, on);
  }
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    return -1;
  }
  ret = ldp_set_allmulti(sockfd, intf->name, on);
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

int ldp_set_mac_addr(int sockfd, const char *ifname, const void *mac);

static inline int ldp_interface_set_mac_addr(struct ldp_interface *intf,
                                             const void *mac)
{
  int ret, sockfd;
  if (intf->mac_addr_set)
  {
    return intf->mac_addr_set(intf, mac);
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
  ret = ldp_set_mac_addr(sockfd, intf->name, mac);
  close(sockfd);
  return ret;
}

#endif
