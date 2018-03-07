#include "ports.h"
#include "odpports.h"
#include <odp_api.h>
#include "time64.h"
#include "mypcapng.h"
#include "log.h"
#include <sys/poll.h>

void odpfunc(struct packet *pkt, void *userdata)
{
  struct odpfunc_userdata *ud = userdata;
  odp_packet_t odppkt;
  char *buf;
  int sent;
  odppkt = odp_packet_alloc(ud->pool, pkt->sz);
  buf = odp_packet_data(odppkt);
  memcpy(buf, packet_data(pkt), pkt->sz);
  sent = odp_pktout_send(ud->outq, &odppkt, 1);
  if (sent < 0)
  {
    sent = 0;
  }
  if (sent == 0)
  {
    odp_packet_free(odppkt);
  }
  allocif_free(ud->intf, pkt);
}

void odpfunc2(struct packet *pkt, void *userdata)
{
  struct odpfunc2_userdata *ud = userdata;
  odp_packet_t odppkt;
  char *buf;
  int sent;
  if (pkt->direction == PACKET_DIRECTION_UPLINK)
  {
    odppkt = odp_packet_alloc(ud->pool, pkt->sz);
    buf = odp_packet_data(odppkt);
    memcpy(buf, packet_data(pkt), pkt->sz);
    sent = odp_pktout_send(ud->uloutq, &odppkt, 1);
    if (sent < 0)
    {
      sent = 0;
    }
    if (sent == 0)
    {
      odp_packet_free(odppkt);
    }
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
    odppkt = odp_packet_alloc(ud->pool, pkt->sz);
    buf = odp_packet_data(odppkt);
    memcpy(buf, packet_data(pkt), pkt->sz);
    sent = odp_pktout_send(ud->dloutq, &odppkt, 1);
    if (sent < 0)
    {
      sent = 0;
    }
    if (sent == 0)
    {
      odp_packet_free(odppkt);
    }
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
