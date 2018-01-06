#define NETMAP_WITH_LIBS
#include "ports.h"
#include "netmapports.h"
#include "net/netmap_user.h"
#include "time64.h"
#include "mypcapng.h"
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
  allocif_free(ud->intf, pkt);
}

void netmapfunc2(struct packet *pkt, void *userdata)
{
  struct netmapfunc2_userdata *ud = userdata;
  if (pkt->direction == PACKET_DIRECTION_UPLINK)
  {
    nm_my_inject(ud->ulnmd, packet_data(pkt), pkt->sz);
    if (ud->wan)
    {
      if (pcapng_out_ctx_write(ud->wanctx, packet_data(pkt), pkt->sz,
          gettime64(), "out"))
      {
        printf("can't record packet\n");
        exit(1);
      }
    }
    if (ud->out)
    {
      if (pcapng_out_ctx_write(ud->outctx, packet_data(pkt), pkt->sz,
          gettime64(), "out"))
      {
        printf("can't record packet\n");
        exit(1);
      }
    }
  }
  else
  {
    nm_my_inject(ud->dlnmd, packet_data(pkt), pkt->sz);
    if (ud->lan)
    {
      if (pcapng_out_ctx_write(ud->lanctx, packet_data(pkt), pkt->sz,
          gettime64(), "out"))
      {
        printf("can't record packet\n");
        exit(1);
      }
    }
    if (ud->out)
    {
      if (pcapng_out_ctx_write(ud->outctx, packet_data(pkt), pkt->sz,
          gettime64(), "in"))
      {
        printf("can't record packet\n");
        exit(1);
      }
    }
  }
  allocif_free(ud->intf, pkt);
}
