#define NETMAP_WITH_LIBS
#include "ports.h"
#include "netmapports.h"
#include "net/netmap_user.h"
#include "netmapcommon.h"
#include "time64.h"
#include "mypcapng.h"
#include "log.h"
#include <sys/poll.h>

void netmapfunc(struct packet *pkt, void *userdata)
{
  struct netmapfunc_userdata *ud = userdata;
  nm_my_inject(ud->nmd, pkt->data, pkt->sz);
  allocif_free(ud->intf, pkt);
}

void netmapfunc2(struct packet *pkt, void *userdata)
{
  struct netmapfunc2_userdata *ud = userdata;
  if (pkt->direction == PACKET_DIRECTION_UPLINK)
  {
    nm_my_inject(ud->ulnmd, pkt->data, pkt->sz);
    if (ud->wan)
    {
      if (pcapng_out_ctx_write(ud->wanctx, pkt->data, pkt->sz,
          gettime64(), "out"))
      {
        log_log(LOG_LEVEL_CRIT, "PORTS", "can't record packet");
        exit(1);
      }
    }
    if (ud->out)
    {
      if (pcapng_out_ctx_write(ud->outctx, pkt->data, pkt->sz,
          gettime64(), "out"))
      {
        log_log(LOG_LEVEL_CRIT, "PORTS", "can't record packet");
        exit(1);
      }
    }
  }
  else
  {
    nm_my_inject(ud->dlnmd, pkt->data, pkt->sz);
    if (ud->lan)
    {
      if (pcapng_out_ctx_write(ud->lanctx, pkt->data, pkt->sz,
          gettime64(), "out"))
      {
        log_log(LOG_LEVEL_CRIT, "PORTS", "can't record packet");
        exit(1);
      }
    }
    if (ud->out)
    {
      if (pcapng_out_ctx_write(ud->outctx, pkt->data, pkt->sz,
          gettime64(), "in"))
      {
        log_log(LOG_LEVEL_CRIT, "PORTS", "can't record packet");
        exit(1);
      }
    }
  }
  allocif_free(ud->intf, pkt);
}
