#ifndef _ODPPORTS_H_
#define _ODPPORTS_H_

#include "packet.h"
#include "allocif.h"
#include "ports.h"
#include <odp_api.h>

void odpfunc(struct packet *pkt, void *userdata);

void odpfunc2(struct packet *pkt, void *userdata);

void odpfunc3(struct packet *pkt, void *userdata);

struct odpfunc_userdata {
  struct allocif *intf;
  odp_pool_t pool;
  odp_pktout_queue_t outq;
};

struct odpfunc2_userdata {
  struct allocif *intf;
  odp_pool_t pool;
  odp_pktout_queue_t uloutq;
  odp_pktout_queue_t dloutq;
  int lan;
  int wan;
  int out;
  struct pcapng_out_ctx *lanctx;
  struct pcapng_out_ctx *wanctx;
  struct pcapng_out_ctx *outctx;
};

struct odpfunc3_userdata {
  struct allocif *intf;
  odp_pool_t pool;
  odp_pktout_queue_t uloutq;
  odp_pktout_queue_t dloutq;
  odp_packet_t uloutbuf[1000];
  odp_packet_t dloutbuf[1000];
  size_t uloutcnt;
  size_t dloutcnt;
  int lan;
  int wan;
  int out;
  struct pcapng_out_ctx *lanctx;
  struct pcapng_out_ctx *wanctx;
  struct pcapng_out_ctx *outctx;
};

void odpfunc3flushdl(struct odpfunc3_userdata *ud);
void odpfunc3flushul(struct odpfunc3_userdata *ud);

#endif
