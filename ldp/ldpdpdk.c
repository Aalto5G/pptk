#define _GNU_SOURCE

#include <sys/poll.h>
#include <sys/socket.h>
#include <linux/ethtool.h>
#include <linux/if_packet.h>
#include <linux/sockios.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <rte_mempool.h>
#include <rte_ethdev.h>
#include <rte_string_fns.h>
#include "ldp.h"
#include "ldpdpdk.h"
#include "linkcommon.h"
#include "containerof.h"

struct ldp_port_dpdk {
  int portid;
  int refc;
};

struct ldp_in_queue_dpdk {
  struct ldp_in_queue q;
  struct ldp_port_dpdk *port;
  int qid;
  int buf_end;
  int buf_start;
  int cache_start; // cache from cache_start to buf_start
  int num_bufs;
  struct rte_mbuf **pkts_all; // FIXME leaks
};

struct ldp_out_queue_dpdk {
  struct ldp_out_queue q;
  int qid;
  struct ldp_port_dpdk *port;
};

struct dpdk_ctx {
  int inited;
  struct rte_mempool *rte_mp;
};

struct dpdk_ctx dpdk_ctx;

static void init_dpdk_ctx(void)
{
  int ret;
  const char *cmdline1 = getenv("LDP_DPDK_ARGV");
  char *cmdline;
#if 0
  char *argv[] = {"foo",
                  /*
                  "-c",
                  "0xf",
                  "-n",
                  "4", */
                  "--vdev=eth_pcap0,iface=veth1",
                  "--vdev=eth_pcap1,iface=veth2",
                  NULL};
#endif
  int argc = 0;
  int len;
  int i;

  if (dpdk_ctx.inited)
  {
    return;
  }
  if (cmdline1 == NULL)
  {
    cmdline1 = "";
  }

  cmdline = strdup(cmdline1);
  len = strlen(cmdline);

  argc = 1;
  for (i = 0; i < len; i++)
  {
    if (isspace(cmdline[i]))
    {
      argc++;
    }
  }

  char *argv[argc];
  argc = rte_strsplit(cmdline, len, argv, argc, ' ');

  ret = rte_eal_init(argc, argv);
  if (ret < 0)
  {
    printf("err\n");
    exit(1);
  }
  dpdk_ctx.rte_mp = rte_pktmbuf_pool_create("mbuf_pool", 8192, 256, 0, 2176, 0 /* socket id */);
  if (dpdk_ctx.rte_mp == NULL)
  {
    printf("err2\n");
    exit(1);
  }
  dpdk_ctx.inited = 1;
}

static void ldp_in_queue_close_dpdk(struct ldp_in_queue *inq)
{
  struct ldp_in_queue_dpdk *innmq;
  innmq = CONTAINER_OF(inq, struct ldp_in_queue_dpdk, q);
  if (--innmq->port->refc == 0)
  {
    rte_eth_dev_stop(innmq->port->portid);
    free(innmq->port);
  }
  free(innmq);
}

static void ldp_out_queue_close_dpdk(struct ldp_out_queue *outq)
{
  struct ldp_out_queue_dpdk *outnmq;
  outnmq = CONTAINER_OF(outq, struct ldp_out_queue_dpdk, q);
  if (--outnmq->port->refc == 0)
  {
    rte_eth_dev_stop(outnmq->port->portid);
    free(outnmq->port);
  }
  free(outnmq);
}

static void ldp_in_queue_deallocate_all_dpdk(struct ldp_in_queue *inq)
{
  struct ldp_in_queue_dpdk *indpdk;
  int i;
  indpdk = CONTAINER_OF(inq, struct ldp_in_queue_dpdk, q);
  i = indpdk->buf_end;
  while (i != indpdk->cache_start)
  {
    rte_pktmbuf_free(indpdk->pkts_all[i]);
    i++;
    if (i >= indpdk->num_bufs)
    {
      i = 0;
    }
  }
  indpdk->buf_end = indpdk->cache_start;
}

static void
ldp_in_queue_deallocate_some_dpdk(struct ldp_in_queue *inq,
                                  struct ldp_packet *pkts, int num)
{
  struct ldp_in_queue_dpdk *indpdk;
  int i;
  indpdk = CONTAINER_OF(inq, struct ldp_in_queue_dpdk, q);
  if (num <= 0)
  {
    return;
  }
  int new_end;
  new_end = pkts[num-1].ancillary + 1;
  if (new_end >= indpdk->num_bufs)
  {
    new_end = 0;
  }
  i = indpdk->buf_end;
  while (i != new_end)
  {
    rte_pktmbuf_free(indpdk->pkts_all[i]);
    i++;
    if (i >= indpdk->num_bufs)
    {
      i = 0;
    }
  }
  indpdk->buf_end = new_end;
}

static int ldp_in_queue_nextpkts_dpdk(struct ldp_in_queue *inq,
                                      struct ldp_packet *pkts, int num)
{
  int nb_rx;
  int max_num;
  int i, j;
  int amnt_free, amnt_cache;
  struct ldp_in_queue_dpdk *indpdkq;

  indpdkq = CONTAINER_OF(inq, struct ldp_in_queue_dpdk, q);
  amnt_cache = indpdkq->buf_start - indpdkq->cache_start;
  if (amnt_cache < 0)
  {
    amnt_cache += indpdkq->num_bufs;
  }

  if (amnt_cache > 0)
  {
    max_num = num;
    if (max_num > amnt_cache)
    {
      max_num = amnt_cache;
    }
    j = indpdkq->cache_start;
    for (i = 0; i < max_num; i++)
    {
      struct rte_mbuf *mbuf = indpdkq->pkts_all[j];
      pkts[i].data = rte_pktmbuf_mtod(mbuf, char *);
      pkts[i].sz = rte_pktmbuf_pkt_len(mbuf);
      pkts[i].ancillary = j;
      j++;
      if (j >= indpdkq->num_bufs)
      {
        j = 0;
      }
    }
    indpdkq->cache_start += max_num;
    if (indpdkq->cache_start >= indpdkq->num_bufs)
    {
      indpdkq->cache_start -= indpdkq->num_bufs;
    }
    
    return max_num;
  }

  max_num = num;
  amnt_free = indpdkq->buf_end - indpdkq->buf_start - 1;
  if (amnt_free < 0)
  {
    amnt_free += indpdkq->num_bufs;
  }
  if (amnt_free == 0)
  {
    return 0;
  }
  if (max_num > amnt_free)
  {
    max_num = amnt_free;
  }
  if (max_num < 4)
  {
    max_num = 4; // some drivers require a minimum burst size
  }
  struct rte_mbuf *local_pkts[max_num];
  nb_rx = rte_eth_rx_burst(indpdkq->port->portid, indpdkq->qid,
                           local_pkts, max_num);
  int nb_rx2 = nb_rx;
  if (nb_rx2 > num)
  {
    nb_rx2 = num;
  }
  for (i = 0; i < nb_rx2; i++)
  {
    struct rte_mbuf *mbuf = local_pkts[i];
    pkts[i].data = rte_pktmbuf_mtod(mbuf, char *);
    pkts[i].sz = rte_pktmbuf_pkt_len(mbuf);
    indpdkq->pkts_all[indpdkq->buf_start] = mbuf;
    pkts[i].ancillary = indpdkq->buf_start++;
    if (indpdkq->buf_start >= indpdkq->num_bufs)
    {
      indpdkq->buf_start = 0;
    }
    //printf("packet received by DPDK, len %zu\n", pkts[i].sz);
  }
  indpdkq->cache_start = indpdkq->buf_start;
  for (i = nb_rx2; i < nb_rx; i++)
  {
    struct rte_mbuf *mbuf = local_pkts[i];
    indpdkq->pkts_all[indpdkq->buf_start] = mbuf;
    indpdkq->buf_start++;
  }
  return nb_rx;
}

static int ldp_out_queue_inject_dpdk(struct ldp_out_queue *outq,
                                     struct ldp_packet *packets, int num)
{
  struct rte_mbuf *tx_mbufs[num];
  int ret;
  int num2;
  int i;
  struct ldp_out_queue_dpdk *outdpdkq;

  outdpdkq = CONTAINER_OF(outq, struct ldp_out_queue_dpdk, q);

  if (rte_pktmbuf_alloc_bulk(dpdk_ctx.rte_mp, tx_mbufs, num))
  {
    //printf("No mbufs alloced\n"); // FIXME rm
    return 0;
  }
  for (i = 0; i < num; i++)
  {
    char *data = rte_pktmbuf_append(tx_mbufs[i], packets[i].sz);
    memcpy(data, packets[i].data, packets[i].sz);
  }

  ret = rte_eth_tx_burst(outdpdkq->port->portid, outdpdkq->qid,
                         tx_mbufs, num);
  num2 = ret;
  if (num2 < 0)
  {
    num2 = 0;
  }
  for (i = num2; i < num; i++)
  {
    rte_pktmbuf_free(tx_mbufs[i]);
  }
  return ret;
}

static int ldp_out_queue_txsync_dpdk(struct ldp_out_queue *outq)
{
  return 0;
}

struct ldp_interface *
ldp_interface_open_dpdk(const char *name, int numinq, int numoutq,
                        const struct ldp_interface_settings *settings)
{
  struct ldp_interface *intf;
  struct ldp_in_queue **inqs;
  struct ldp_out_queue **outqs;
  int i;
  int ret;
  struct ldp_port_dpdk *port;
  long int portid;
  char *endptr;
  struct rte_eth_conf port_conf = {};
  uint16_t nb_rxd = 128;
  uint16_t nb_txd = 512;


  port_conf.rxmode.hw_strip_crc = 1;
  port_conf.txmode.mq_mode = ETH_MQ_TX_NONE;

  if (numinq < 0 || numoutq < 0 || numinq > 1024*1024 || numoutq > 1024*1024)
  {
    abort();
  }
  portid = strtol(name, &endptr, 10);
  if (*name == '\0' || *endptr != '\0' || portid < 0 || portid > INT_MAX)
  {
    printf("1\n");
    return NULL;
  }

  init_dpdk_ctx();
  
  if (!rte_eth_dev_is_valid_port(portid))
  {
    printf("invalid port\n");
    return NULL;
  }
  ret = rte_eth_dev_configure(portid, numinq, numoutq, &port_conf);
  if (ret < 0)
  {
    printf("2: %d\n", ret);
    return NULL;
  }
  ret = rte_eth_dev_adjust_nb_rx_tx_desc(portid, &nb_rxd, &nb_txd);
  if (ret < 0)
  {
    printf("3\n");
    return NULL;
  }
  for (i = 0; i < numinq; i++)
  {
    ret = rte_eth_rx_queue_setup(portid, i, nb_rxd,
                                 rte_eth_dev_socket_id(portid),
                                 NULL, dpdk_ctx.rte_mp);
    if (ret < 0)
    {
      printf("4\n");
      return NULL;
    }
  }
  for (i = 0; i < numinq; i++)
  {
    ret = rte_eth_tx_queue_setup(portid, i, nb_txd,
                                 rte_eth_dev_socket_id(portid), NULL);
    if (ret < 0)
    {
      printf("5\n");
      return NULL;
    }
  }
  if (settings && settings->mtu_set)
  {
    if (rte_eth_dev_set_mtu(portid, settings->mtu) != 0)
    {
      printf("5.5\n");
      return NULL;
    }
  }
  ret = rte_eth_dev_start(portid);
  if (ret < 0)
  {
    printf("6\n");
    return NULL;
  }


  port = malloc(sizeof(*port));
  if (port == NULL)
  {
    abort(); // FIXME better error handling
  }
  port->refc = numinq + numoutq;
  port->portid = portid;
  intf = malloc(sizeof(*intf));
  if (intf == NULL)
  {
    abort(); // FIXME better error handling
  }
  intf->promisc_mode_set = NULL;
  intf->allmulti_set = NULL;
  intf->link_wait = NULL;
  intf->link_status = NULL;
  intf->mac_addr = NULL;
  intf->mac_addr_set = NULL;
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
    struct ldp_in_queue_dpdk *innmq;
    innmq = malloc(sizeof(*innmq));
    if (innmq == NULL)
    {
      abort(); // FIXME better error handling
    }
    inqs[i] = &innmq->q;
  }
  for (i = 0; i < numoutq; i++)
  {
    struct ldp_out_queue_dpdk *outnmq;
    outnmq = malloc(sizeof(*outnmq));
    if (outnmq == NULL)
    {
      abort(); // FIXME better error handling
    }
    outqs[i] = &outnmq->q;
  }
  for (i = 0; i < numinq; i++)
  {
    struct ldp_in_queue_dpdk *innmq;
    innmq = CONTAINER_OF(inqs[i], struct ldp_in_queue_dpdk, q);
    innmq->port = port;
    innmq->buf_start = 0;
    innmq->cache_start = 0;
    innmq->buf_end = 0;
    innmq->num_bufs = 128;
    innmq->pkts_all = malloc(innmq->num_bufs*sizeof(*innmq->pkts_all));
    if (innmq->pkts_all == NULL)
    {
      abort(); // FIXME better error handling
    }
    innmq->qid = i;
    innmq->q.nextpkts = ldp_in_queue_nextpkts_dpdk;
    innmq->q.nextpkts_ts = NULL;
    innmq->q.poll = ldp_in_queue_poll;
    innmq->q.eof = NULL;
    innmq->q.close = ldp_in_queue_close_dpdk;
    innmq->q.deallocate_all = ldp_in_queue_deallocate_all_dpdk;
    innmq->q.deallocate_some = ldp_in_queue_deallocate_some_dpdk;
    innmq->q.fd = -1;
  }
  for (i = 0; i < numoutq; i++)
  {
    struct ldp_out_queue_dpdk *outnmq;
    outnmq = CONTAINER_OF(outqs[i], struct ldp_out_queue_dpdk, q);
    outnmq->port = port;
    outnmq->qid = i;
    outnmq->q.inject = ldp_out_queue_inject_dpdk;
    outnmq->q.txsync = ldp_out_queue_txsync_dpdk;
    outnmq->q.close = ldp_out_queue_close_dpdk;
    outnmq->q.fd = -1;
  }
  intf->num_inq = numinq;
  intf->num_outq = numoutq;
  intf->inq = inqs;
  intf->outq = outqs;
  snprintf(intf->name, sizeof(intf->name), "%s", name);
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

int ldp_dpdk_mac_addr(int portid, void *mac)
{
  rte_eth_macaddr_get(portid, (struct ether_addr *)mac);
  return 0;
}

int ldp_dpdk_set_mac_addr(int portid, const void *mac)
{
  char mac2[6];
  memcpy(mac2, mac, 6);
  return rte_eth_dev_default_mac_addr_set(portid, (struct ether_addr *)mac2);
}

int ldp_dpdk_promisc_mode_set(int portid, int on)
{
  if (on)
  {
    rte_eth_promiscuous_enable(portid);
  }
  else
  {
    rte_eth_promiscuous_disable(portid);
  }
  return 0;
}

int ldp_dpdk_allmulti_set(int portid, int on)
{
  if (on)
  {
    rte_eth_allmulticast_enable(portid);
  }
  else
  {
    rte_eth_allmulticast_disable(portid);
  }
  return 0;
}
