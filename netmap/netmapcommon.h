#ifndef _NETMAPCOMMON_H_
#define _NETMAPCOMMON_H_

#include <sys/poll.h>
#include "branchpredict.h"

static inline void nm_my_inject(struct nm_desc *nmd, void *data, size_t sz)
{
  if (likely(nm_inject(nmd, data, sz) != 0))
  {
    return;
  }
  struct pollfd pollfd;
  pollfd.fd = nmd->fd;
  pollfd.events = POLLOUT;
  poll(&pollfd, 1, -1);
  nm_inject(nmd, data, sz);
}

void set_promisc_mode(int sockfd, const char *ifname, int on);

int link_status(int sockfd, const char *ifname);

void link_wait(int sockfd, const char *ifname);

#endif
