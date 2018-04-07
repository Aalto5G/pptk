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
#include "containerof.h"

struct ldp_port_dpdk {
  int portid;
  int refc;
};

struct ldp_in_queue_dpdk {
  struct ldp_in_queue q;
  struct ldp_port_dpdk *port;
  int minptr;
  int num;
  int qid;
  struct rte_mbuf *pkts_burst[128];
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


static int ldp_in_queue_nextpkts_dpdk(struct ldp_in_queue *inq,
                                      struct ldp_packet *pkts, int num)
{
  int nb_rx;
  int max_num;
  int i;
  struct ldp_in_queue_dpdk *indpdkq;

  indpdkq = CONTAINER_OF(inq, struct ldp_in_queue_dpdk, q);

  if (indpdkq->minptr < indpdkq->num)
  {
    max_num = num;
    if (max_num > indpdkq->num - indpdkq->minptr)
    {
      max_num = indpdkq->num - indpdkq->minptr;
    }
    for (i = 0; i < max_num; i++)
    {
      struct rte_mbuf *mbuf = indpdkq->pkts_burst[indpdkq->minptr + i];
      pkts[i].data = rte_pktmbuf_mtod(mbuf, char *);
      pkts[i].sz = rte_pktmbuf_pkt_len(mbuf);
    }
    indpdkq->minptr += max_num;
    return max_num;
  }

  for (i = 0; i < indpdkq->num; i++)
  {
    struct rte_mbuf *mbuf = indpdkq->pkts_burst[i];
    rte_pktmbuf_free(mbuf);
    indpdkq->pkts_burst[i] = NULL;
  }
  indpdkq->num = 0;
  indpdkq->minptr = 0;

  max_num = num;
  if (max_num > (int)(sizeof(indpdkq->pkts_burst)/sizeof(*indpdkq->pkts_burst)))
  {
    max_num = sizeof(indpdkq->pkts_burst)/sizeof(*indpdkq->pkts_burst);
  }
  if (max_num < 4)
  {
    max_num = 4; // some drivers require a minimum burst size
  }
  nb_rx = rte_eth_rx_burst(indpdkq->port->portid, indpdkq->qid,
                           indpdkq->pkts_burst,
                           max_num);
  indpdkq->num = nb_rx;
  if (nb_rx > num)
  {
    nb_rx = num;
  }
  indpdkq->minptr = nb_rx;
  for (i = 0; i < nb_rx; i++)
  {
    struct rte_mbuf *mbuf = indpdkq->pkts_burst[i];
    pkts[i].data = rte_pktmbuf_mtod(mbuf, char *);
    pkts[i].sz = rte_pktmbuf_pkt_len(mbuf);
    //printf("packet received by DPDK, len %zu\n", pkts[i].sz);
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
ldp_interface_open_dpdk(const char *name, int numinq, int numoutq)
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
    innmq->num = 0;
    innmq->minptr = 0;
    innmq->qid = i;
    innmq->q.nextpkts = ldp_in_queue_nextpkts_dpdk;
    innmq->q.poll = ldp_in_queue_poll;
    innmq->q.close = ldp_in_queue_close_dpdk;
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
  return intf;
}

int ldp_dpdk_mac_addr(int portid, void *mac)
{
  rte_eth_macaddr_get(portid, (struct ether_addr *)mac);
  return 0;
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
