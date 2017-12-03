#define NETMAP_WITH_LIBS
#include "ports.h"
#include "netmapports.h"
#include "net/netmap_user.h"
#include <sys/poll.h>

static inline void nm_my_inject(struct nm_desc *nmd, void *data, size_t sz)
{
  int i, j;
  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < 2; j++)
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

void netmapfunc(struct packet *pkt, void *userdata)
{
  struct netmapfunc_userdata *ud = userdata;
  nm_my_inject(ud->nmd, packet_data(pkt), pkt->sz);
  as_free_mt(ud->loc, pkt);
}

void netmapfunc2(struct packet *pkt, void *userdata)
{
  struct netmapfunc2_userdata *ud = userdata;
  if (pkt->direction == PACKET_DIRECTION_UPLINK)
  {
    nm_my_inject(ud->ulnmd, packet_data(pkt), pkt->sz);
  }
  else
  {
    nm_my_inject(ud->dlnmd, packet_data(pkt), pkt->sz);
  }
  as_free_mt(ud->loc, pkt);
}
