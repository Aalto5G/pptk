#define _GNU_SOURCE

#include <sys/poll.h>
#include <sys/socket.h>
#include <linux/ethtool.h>
#include <linux/if_packet.h>
#include <linux/sockios.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <odp_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ldp.h"
#include "ldpodp.h"
#include "linkcommon.h"
#include "containerof.h"

#define POOL_NUM_PKT 1024
#define POOL_SEG_LEN 1856

struct ldp_port_odp {
  odp_pktio_t pktio;
  int refc;
};

struct ldp_in_queue_odp {
  struct ldp_in_queue q;
  struct ldp_port_odp *port;
  odp_pktin_queue_t odpq;
  int buf_end;
  int buf_start;
  int num_bufs;
  odp_packet_t *pkts_all; // FIXME leaks
};

static uint32_t ldp_in_queue_ring_size_odp(struct ldp_in_queue *inq)
{
  struct ldp_in_queue_odp *inodp;
  inodp = CONTAINER_OF(inq, struct ldp_in_queue_odp, q);
  return inodp->num_bufs;
}

struct ldp_out_queue_odp {
  struct ldp_out_queue q;
  odp_pktout_queue_t odpq;
  struct ldp_port_odp *port;
};

struct odp_ctx {
  int inited;
  odp_pool_t pool;
  odp_instance_t instance;
};
__thread int odp_thr_inited;

struct odp_ctx odp_ctx;

static inline int odp_check_thread_init(void)
{
  if (odp_thr_inited)
  {
    return 0;
  }
  printf("initing ODP thread\n");
  if (odp_init_local(odp_ctx.instance, ODP_THREAD_WORKER))
  {
    return -1;
  }
  odp_thr_inited = 1;
  return 0;
}

static int ldp_odp_mac_addr(struct ldp_in_queue *q, void *mac)
{
  struct ldp_in_queue_odp *odpq = CONTAINER_OF(q, struct ldp_in_queue_odp, q);
  return odp_pktio_mac_addr(odpq->port->pktio, mac, 6);
}

static int ldp_odp_promisc_mode_set(struct ldp_in_queue *q, int on)
{
  struct ldp_in_queue_odp *odpq = CONTAINER_OF(q, struct ldp_in_queue_odp, q);
  return odp_pktio_promisc_mode_set(odpq->port->pktio, on);
}

static int ldp_odp_mac_addr_2(struct ldp_interface *intf, void *mac)
{
  return ldp_odp_mac_addr(intf->inq[0], mac);
}

static int ldp_odp_promisc_mode_set_2(struct ldp_interface *intf, int on)
{
  return ldp_odp_promisc_mode_set(intf->inq[0], on);
}

static int ldp_odp_link_wait_2(struct ldp_interface *intf)
{
  return 0;
}

static int ldp_odp_link_status_2(struct ldp_interface *intf)
{
  return 1;
}

static odp_pktio_t
ldp_create_pktio_multiqueue(const char *name,
                            odp_pktin_queue_t *inq, odp_pktout_queue_t *outq,
                            int numinq, int numoutq)
{
  odp_pktio_param_t pktio_param;
  odp_pktin_queue_param_t in_queue_param;
  odp_pktout_queue_param_t out_queue_param;
  odp_pktio_t pktio;
  odp_pktio_config_t config;

  odp_check_thread_init();

  odp_pktio_param_init(&pktio_param);
  pktio = odp_pktio_open(name, odp_ctx.pool, &pktio_param);
  if (pktio == ODP_PKTIO_INVALID)
  {
    return ODP_PKTIO_INVALID;
  }

  odp_pktio_config_init(&config);
  config.parser.layer = ODP_PROTO_LAYER_L2;
  odp_pktio_config(pktio, &config);

  odp_pktin_queue_param_init(&in_queue_param);
  odp_pktout_queue_param_init(&out_queue_param);

  in_queue_param.op_mode = ODP_PKTIO_OP_MT_UNSAFE;

  if (odp_pktin_queue_config(pktio, &in_queue_param))
  {
    odp_pktio_close(pktio);
    return ODP_PKTIO_INVALID;
  }

  out_queue_param.op_mode = ODP_PKTIO_OP_MT_UNSAFE;

  if (odp_pktout_queue_config(pktio, &out_queue_param))
  {
    odp_pktio_close(pktio);
    return ODP_PKTIO_INVALID;
  }

  if (odp_pktin_queue(pktio, inq, numinq) != numinq)
  {
    odp_pktio_close(pktio);
    return ODP_PKTIO_INVALID;
  }
  if (odp_pktout_queue(pktio, outq, numoutq) != numoutq)
  {
    odp_pktio_close(pktio);
    return ODP_PKTIO_INVALID;
  }
  if (odp_pktio_start(pktio))
  {
    odp_pktio_close(pktio);
    return ODP_PKTIO_INVALID;
  }
  return pktio;
}

static int init_odp_ctx(void)
{
  odp_pool_param_t params;

  if (odp_ctx.inited)
  {
    return 0;
  }

  if (odp_init_global(&odp_ctx.instance, NULL, NULL))
  {
    return -1;
  }

  odp_check_thread_init();

  odp_pool_param_init(&params);
  params.pkt.seg_len = POOL_SEG_LEN;
  params.pkt.len = POOL_SEG_LEN;
  params.pkt.num = POOL_NUM_PKT;
  params.type = ODP_POOL_PACKET;
  odp_ctx.pool = odp_pool_create("packet pool", &params);
  if (odp_ctx.pool == ODP_POOL_INVALID)
  {
    return -1;
  }
  odp_ctx.inited = 1;
  return 0;
}

static void ldp_in_queue_close_odp(struct ldp_in_queue *inq)
{
  struct ldp_in_queue_odp *innmq;
  innmq = CONTAINER_OF(inq, struct ldp_in_queue_odp, q);
  if (--innmq->port->refc == 0)
  {
    odp_pktio_stop(innmq->port->pktio);
    odp_pktio_close(innmq->port->pktio);
    free(innmq->port);
  }
  free(innmq);
}

static void ldp_out_queue_close_odp(struct ldp_out_queue *outq)
{
  struct ldp_out_queue_odp *outnmq;
  outnmq = CONTAINER_OF(outq, struct ldp_out_queue_odp, q);
  if (--outnmq->port->refc == 0)
  {
    odp_pktio_stop(outnmq->port->pktio);
    odp_pktio_close(outnmq->port->pktio);
    free(outnmq->port);
  }
  free(outnmq);
}

static void ldp_in_queue_deallocate_all_odp(struct ldp_in_queue *inq)
{
  struct ldp_in_queue_odp *inodp;
  int i; 
  inodp = CONTAINER_OF(inq, struct ldp_in_queue_odp, q);
  i = inodp->buf_end;
  while (i != inodp->buf_start)
  { 
    odp_packet_free(inodp->pkts_all[i]);
    i++;
    if (i >= inodp->num_bufs)
    { 
      i = 0;
    }
  }
  inodp->buf_end = inodp->buf_start;
}

static void
ldp_in_queue_deallocate_some_odp(struct ldp_in_queue *inq,
                                  struct ldp_packet *pkts, int num)
{
  struct ldp_in_queue_odp *inodp;
  int i;
  inodp = CONTAINER_OF(inq, struct ldp_in_queue_odp, q);
  if (num <= 0)
  {
    return;
  }
  int new_end;
  new_end = pkts[num-1].ancillary + 1;
  if (new_end >= inodp->num_bufs)
  {
    new_end = 0;
  }
  i = inodp->buf_end;
  while (i != new_end)
  {
    odp_packet_free(inodp->pkts_all[i]);
    i++;
    if (i >= inodp->num_bufs)
    {
      i = 0;
    }
  }
  inodp->buf_end = new_end;
}


static int ldp_in_queue_nextpkts_odp(struct ldp_in_queue *inq,
                                     struct ldp_packet *pkts, int num)
{
  int nb_rx;
  int max_num;
  int i;
  int amnt_free;
  struct ldp_in_queue_odp *inodpq;

  odp_check_thread_init();

  inodpq = CONTAINER_OF(inq, struct ldp_in_queue_odp, q);

  amnt_free = inodpq->buf_end - inodpq->buf_start - 1;
  if (amnt_free < 0)
  {
    amnt_free += inodpq->num_bufs;
  }
  if (amnt_free == 0)
  {
    return 0;
  }

  max_num = num;
  if (max_num > amnt_free)
  {
    max_num = amnt_free;
  }
  odp_packet_t local_pkts[max_num];
  nb_rx = odp_pktin_recv(inodpq->odpq, local_pkts, max_num);
  if (nb_rx <= 0)
  {
    return nb_rx;
  }
  for (i = 0; i < nb_rx; i++)
  {
    odp_packet_t mbuf = local_pkts[i];
    pkts[i].data = odp_packet_data(mbuf);
    pkts[i].sz = odp_packet_len(mbuf);
    inodpq->pkts_all[inodpq->buf_start] = mbuf;
    pkts[i].ancillary = inodpq->buf_start++;
    if (inodpq->buf_start >= inodpq->num_bufs)
    {
      inodpq->buf_start = 0;
    }
    //printf("packet received by DPDK, len %zu\n", pkts[i].sz);
  }
  return nb_rx;
}

static int ldp_out_queue_inject_odp(struct ldp_out_queue *outq,
                                     struct ldp_packet *packets, int num)
{
  odp_packet_t tx_mbufs[num];
  int ret;
  int num2;
  int i;
  struct ldp_out_queue_odp *outodpq;

  odp_check_thread_init();

  outodpq = CONTAINER_OF(outq, struct ldp_out_queue_odp, q);

  for (i = 0; i < num; i++)
  {
    tx_mbufs[i] = odp_packet_alloc(odp_ctx.pool, packets[i].sz);
    memcpy(odp_packet_data(tx_mbufs[i]), packets[i].data, packets[i].sz);
  }

  ret = odp_pktout_send(outodpq->odpq, tx_mbufs, num);
  num2 = ret;
  if (num2 < 0)
  {
    num2 = 0;
  }
  for (i = num2; i < num; i++)
  {
    odp_packet_free(tx_mbufs[i]);
  }
  return ret;
}

static int ldp_out_queue_txsync_odp(struct ldp_out_queue *outq)
{
  return 0;
}

struct ldp_interface *
ldp_interface_open_odp(const char *name, int numinq, int numoutq,
                       const struct ldp_interface_settings *settings)
{
  struct ldp_interface *intf;
  struct ldp_in_queue **inqs;
  struct ldp_out_queue **outqs;
  odp_pktin_queue_t odpinqs[numinq];
  odp_pktout_queue_t odpoutqs[numoutq];
  odp_pktio_t pktio;
  int i;
  struct ldp_port_odp *port;

  if (numinq < 0 || numoutq < 0 || numinq > 1024*1024 || numoutq > 1024*1024)
  {
    abort();
  }

  init_odp_ctx();
  odp_check_thread_init();

  pktio = ldp_create_pktio_multiqueue(name, odpinqs, odpoutqs, numinq, numoutq);
  
  port = malloc(sizeof(*port));
  if (port == NULL)
  {
    abort(); // FIXME better error handling
  }
  port->refc = numinq + numoutq;
  port->pktio = pktio;
  intf = malloc(sizeof(*intf));
  if (intf == NULL)
  {
    abort(); // FIXME better error handling
  }
  intf->mac_addr = ldp_odp_mac_addr_2;
  intf->promisc_mode_set = ldp_odp_promisc_mode_set_2;
  intf->link_wait = ldp_odp_link_wait_2;
  intf->link_status = ldp_odp_link_status_2;
  inqs = malloc(numinq*sizeof(*inqs));
  if (inqs == NULL)
  {
    abort(); // FIXME better error handling
  }
  outqs = malloc(numoutq*sizeof(*outqs));
  if (outqs == NULL)
  {
    abort(); // FIXME better error handling
  }
  for (i = 0; i < numinq; i++)
  {
    struct ldp_in_queue_odp *innmq;
    innmq = malloc(sizeof(*innmq));
    if (innmq == NULL)
    {
      abort(); // FIXME better error handling
    }
    inqs[i] = &innmq->q;
  }
  for (i = 0; i < numoutq; i++)
  {
    struct ldp_out_queue_odp *outnmq;
    outnmq = malloc(sizeof(*outnmq));
    if (outnmq == NULL)
    {
      abort(); // FIXME better error handling
    }
    outqs[i] = &outnmq->q;
  }
  for (i = 0; i < numinq; i++)
  {
    struct ldp_in_queue_odp *innmq;
    innmq = CONTAINER_OF(inqs[i], struct ldp_in_queue_odp, q);
    innmq->port = port;
    innmq->num_bufs = 128;
    innmq->pkts_all = malloc(innmq->num_bufs*sizeof(*innmq->pkts_all));
    innmq->buf_start = 0;
    innmq->buf_end = 0;
    innmq->odpq = odpinqs[i];
    innmq->q.nextpkts = ldp_in_queue_nextpkts_odp;
    innmq->q.nextpkts_ts = NULL;
    innmq->q.poll = ldp_in_queue_poll;
    innmq->q.eof = NULL;
    innmq->q.close = ldp_in_queue_close_odp;
    innmq->q.deallocate_all = ldp_in_queue_deallocate_all_odp;
    innmq->q.deallocate_some = ldp_in_queue_deallocate_some_odp;
    innmq->q.ring_size = ldp_in_queue_ring_size_odp;
    innmq->q.fd = -1;
  }
  for (i = 0; i < numoutq; i++)
  {
    struct ldp_out_queue_odp *outnmq;
    outnmq = CONTAINER_OF(outqs[i], struct ldp_out_queue_odp, q);
    outnmq->port = port;
    outnmq->odpq = odpoutqs[i];
    outnmq->q.inject = ldp_out_queue_inject_odp;
    outnmq->q.txsync = ldp_out_queue_txsync_odp;
    outnmq->q.close = ldp_out_queue_close_odp;
    outnmq->q.fd = -1;
  }
  intf->num_inq = numinq;
  intf->num_outq = numoutq;
  intf->inq = inqs;
  intf->outq = outqs;
  snprintf(intf->name, sizeof(intf->name), "%s", name);
  if (settings && settings->mtu_set)
  {
#if ODP_VERSION_API_MAJOR >= 17
    if (odp_pktin_maxlen(pktio) != settings->mtu)
    {
      abort(); // FIXME better error handling
    }
#else
    if (odp_pktio_mtu(pktio) != settings->mtu)
    {
      abort(); // FIXME better error handling
    }
#endif
  }
  if (settings && settings->promisc_set)
  {
    ldp_interface_set_promisc_mode(intf, settings->promisc_on);
  }
  if (settings && settings->allmulti_set)
  {
    ldp_interface_set_allmulti(intf, settings->allmulti_on);
  }
  if (settings && settings->mac_addr_set)
  {
    ldp_interface_set_mac_addr(intf, settings->mac);
  }
  return intf;
}

