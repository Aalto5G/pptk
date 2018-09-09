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
  memcpy(buf, pkt->data, pkt->sz);
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
    memcpy(buf, pkt->data, pkt->sz);
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
    odppkt = odp_packet_alloc(ud->pool, pkt->sz);
    buf = odp_packet_data(odppkt);
    memcpy(buf, pkt->data, pkt->sz);
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

void odpfunc3flushdl(struct odpfunc3_userdata *ud)
{
  int sent;
  int lost;
  sent = odp_pktout_send(ud->dloutq, ud->dloutbuf, ud->dloutcnt);
  if (sent < 0)
  {
    sent = 0;
  }
  lost = ((int)ud->dloutcnt) - sent;
  ud->dloutcnt = 0;
  odp_packet_free_multi(&ud->dloutbuf[sent], lost);
}

void odpfunc3flushul(struct odpfunc3_userdata *ud)
{
  int sent;
  int lost;
  sent = odp_pktout_send(ud->uloutq, ud->uloutbuf, ud->uloutcnt);
  if (sent < 0)
  {
    sent = 0;
  }
  lost = ((int)ud->uloutcnt) - sent;
  ud->uloutcnt = 0;
  odp_packet_free_multi(&ud->uloutbuf[sent], lost);
}

void odpfunc3(struct packet *pkt, void *userdata)
{
  struct odpfunc3_userdata *ud = userdata;
  odp_packet_t odppkt;
  char *buf;
  if (pkt->direction == PACKET_DIRECTION_UPLINK)
  {
    odppkt = odp_packet_alloc(ud->pool, pkt->sz);
    buf = odp_packet_data(odppkt);
    memcpy(buf, pkt->data, pkt->sz);
    if (ud->uloutcnt >= sizeof(ud->uloutbuf)/sizeof(*ud->uloutbuf))
    {
      odpfunc3flushul(ud);
    }
    if (ud->uloutcnt >= sizeof(ud->uloutbuf)/sizeof(*ud->uloutbuf))
    {
      abort();
    }
    ud->uloutbuf[ud->uloutcnt++] = odppkt;
#if 0
    sent = odp_pktout_send(ud->uloutq, &odppkt, 1);
    if (sent < 0)
    {
      sent = 0;
    }
    if (sent == 0)
    {
      odp_packet_free(odppkt);
    }
#endif
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
    odppkt = odp_packet_alloc(ud->pool, pkt->sz);
    buf = odp_packet_data(odppkt);
    memcpy(buf, pkt->data, pkt->sz);
    if (ud->dloutcnt >= sizeof(ud->dloutbuf)/sizeof(*ud->dloutbuf))
    {
      odpfunc3flushdl(ud);
    }
    if (ud->dloutcnt >= sizeof(ud->dloutbuf)/sizeof(*ud->dloutbuf))
    {
      abort();
    }
    ud->dloutbuf[ud->dloutcnt++] = odppkt;
#if 0
    sent = odp_pktout_send(ud->dloutq, &odppkt, 1);
    if (sent < 0)
    {
      sent = 0;
    }
    if (sent == 0)
    {
      odp_packet_free(odppkt);
    }
#endif
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
