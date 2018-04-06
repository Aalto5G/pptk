#include "ports.h"
#include "ldpports.h"
#include <ldp_api.h>
#include "time64.h"
#include "mypcapng.h"
#include "log.h"
#include <sys/poll.h>

void ldpfunc(struct packet *pkt, void *userdata)
{
  struct ldpfunc_userdata *ud = userdata;
  char *buf;
  struct ldp_packet pkts[1];
  int sent;
  pkts[0].data = packet_data(pkt);
  pkts[0].sz = pkt->sz;
  ldp_out_inject(ud->outq, pkts, 1);
  allocif_free(ud->intf, pkt);
}

void ldpfunc2(struct packet *pkt, void *userdata)
{
  struct ldpfunc2_userdata *ud = userdata;
  struct ldp_packet pkts[1];
  char *buf;
  int sent;
  pkts[0].data = packet_data(pkt);
  pkts[0].sz = pkt->sz;
  if (pkt->direction == PACKET_DIRECTION_UPLINK)
  {
    ldp_out_inject(ud->uloutq, pkts, 1);
    if (ud->wan)
    {
      if (pcapng_out_ctx_write(ud->wanctx, packet_data(pkt), pkt->sz,
          gettime64(), "out"))
      {
        log_log(LOG_LEVEL_CRIT, "PORTS", "can't record packet");
        exit(1);
      }
    }
    if (ud->out)
    {
      if (pcapng_out_ctx_write(ud->outctx, packet_data(pkt), pkt->sz,
          gettime64(), "out"))
      {
        log_log(LOG_LEVEL_CRIT, "PORTS", "can't record packet");
        exit(1);
      }
    }
  }
  else
  {
    ldp_out_inject(ud->dloutq, pkts, 1);
    if (ud->lan)
    {
      if (pcapng_out_ctx_write(ud->lanctx, packet_data(pkt), pkt->sz,
          gettime64(), "out"))
      {
        log_log(LOG_LEVEL_CRIT, "PORTS", "can't record packet");
        exit(1);
      }
    }
    if (ud->out)
    {
      if (pcapng_out_ctx_write(ud->outctx, packet_data(pkt), pkt->sz,
          gettime64(), "in"))
      {
        log_log(LOG_LEVEL_CRIT, "PORTS", "can't record packet");
        exit(1);
      }
    }
  }
  allocif_free(ud->intf, pkt);
}
