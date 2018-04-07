#ifndef _LINKCOMMON_H_
#define _LINKCOMMON_H_

#include <errno.h>
#include "ldp.h"

int ldp_set_promisc_mode(int sockfd, const char *ifname, int on);

static inline int
ldp_interface_set_promisc_mode(struct ldp_interface *intf, int on)
{
  int ret, sockfd;
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
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    return -ENETDOWN;
  }
  ret = ldp_link_wait(sockfd, intf->name);
  close(sockfd);
  return ret;
}

#endif
