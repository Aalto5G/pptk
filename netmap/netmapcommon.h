#ifndef _NETMAPCOMMON_H_
#define _NETMAPCOMMON_H_

#include <sys/poll.h>

static inline void nm_my_inject(struct nm_desc *nmd, void *data, size_t sz)
{
  int i, j;
  for (i = 0; i < 3; i++)
  {
    for (j = 0; j < 3; j++)
    {
      if (nm_inject(nmd, data, sz) == 0)
      {
        struct pollfd pollfd;
        pollfd.fd = nmd->fd;
        pollfd.events = POLLOUT;
        poll(&pollfd, 1, 0);
      }
      else
      {
        return;
      }
    }
    ioctl(nmd->fd, NIOCTXSYNC, NULL);
  }
}

void set_promisc_mode(int sockfd, const char *ifname, int on);

int link_status(int sockfd, const char *ifname);

void link_wait(int sockfd, const char *ifname);

#endif
