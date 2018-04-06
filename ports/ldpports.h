#ifndef _ODPPORTS_H_
#define _ODPPORTS_H_

#include "packet.h"
#include "allocif.h"
#include "ports.h"
#include "ldp.h"

void ldpfunc(struct packet *pkt, void *userdata);

void ldpfunc2(struct packet *pkt, void *userdata);

struct ldpfunc_userdata {
  struct allocif *intf;
  struct ldp_out_queue *outq;
};

struct ldpfunc2_userdata {
  struct allocif *intf;
  struct ldp_out_queue *uloutq;
  struct ldp_out_queue *dloutq;
  int lan;
  int wan;
  int out;
  struct pcapng_out_ctx *lanctx;
  struct pcapng_out_ctx *wanctx;
  struct pcapng_out_ctx *outctx;
};

struct ldpfunc3_userdata {
  struct allocif *intf;
  struct ldp_out_queue *uloutq;
  struct ldp_out_queue *dloutq;
  struct packet *uloutbuf[1000];
  struct packet *dloutbuf[1000];
  size_t uloutcnt;
  size_t dloutcnt;
  int lan;
  int wan;
  int out;
  struct pcapng_out_ctx *lanctx;
  struct pcapng_out_ctx *wanctx;
  struct pcapng_out_ctx *outctx;
};

void ldpfunc3flushul(struct ldpfunc3_userdata *ud);

void ldpfunc3flushdl(struct ldpfunc3_userdata *ud);

void ldpfunc3(struct packet *pkt, void *userdata);

#endif
