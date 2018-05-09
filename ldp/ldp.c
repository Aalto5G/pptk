#define _GNU_SOURCE

#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/ethtool.h>
#include <linux/if_packet.h>
#include <linux/sockios.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <arpa/inet.h>
#include "ldp.h"
#include "ldpnull.h"
#include "linkcommon.h"
#include "ldppcap.h"
#include "containerof.h"
#if WITH_NETMAP
#include "ldpnetmap.h"
#endif
#if WITH_DPDK
#include "ldpdpdk.h"
#endif
#if WITH_ODP
#include "ldpodp.h"
#endif

static int ldp_config_global_isset = 0;
static struct ldp_config ldp_config_global = {};

void ldp_config_init(struct ldp_config *config)
{
  memset(config, 0, sizeof(*config));
  config->netmap_nr_rx_slots = 256;
  config->netmap_nr_tx_slots = 64;
  config->dpdk_pool_num = 8192;
  config->dpdk_pool_cache_num = 256;
  config->dpdk_pool_data_room = 2176;
  config->socket_num_bufs = 1024;
  config->odp_num_pkt = 8192;
  config->odp_pkt_len = 1856;
  config->pcap_num_bufs = 1024;
}

struct ldp_config *ldp_config_get_global(void)
{
  if (!ldp_config_global_isset)
  {
    struct ldp_config ldp_config_local = {};
    ldp_config_init(&ldp_config_local);
    ldp_config_set(&ldp_config_local);
  }
  if (!ldp_config_global_isset)
  {
    abort();
  }
  return &ldp_config_global;
}

// Must be called before calling any other LDP function, if default is not ok
void ldp_config_set(struct ldp_config *config)
{
  ldp_config_global = *config; // struct assignment
  ldp_config_global_isset = 1;
}

struct ldp_in_queue_socket {
  struct ldp_in_queue q;
  int max_sz;
  int num_bufs;
  int buf_start;
  int buf_end;
  char **bufs; // always at least 1 slot free
};

static uint32_t ldp_in_queue_ring_size_socket(struct ldp_in_queue *inq)
{
  struct ldp_in_queue_socket *insock;
  insock = CONTAINER_OF(inq, struct ldp_in_queue_socket, q);
  return insock->num_bufs;
}

struct ldp_out_queue_socket {
  struct ldp_out_queue q;
};

static void ldp_in_queue_close_socket(struct ldp_in_queue *inq)
{
  int i;
  struct ldp_in_queue_socket *insock;
  insock = CONTAINER_OF(inq, struct ldp_in_queue_socket, q);
  close(insock->q.fd);
  for (i = 0; i < insock->num_bufs; i++)
  {
    free(insock->bufs[i]);
  }
  free(insock->bufs);
  free(insock);
}

static void ldp_out_queue_close_socket(struct ldp_out_queue *outq)
{
  struct ldp_out_queue_socket *outsock;
  outsock = CONTAINER_OF(outq, struct ldp_out_queue_socket, q);
  close(outsock->q.fd);
  free(outsock);
}

static int ldp_out_queue_txsync_socket(struct ldp_out_queue *outq)
{
  return 0;
}



int ldp_in_queue_poll(struct ldp_in_queue *inq, uint64_t timeout_usec)
{
  int nfds;
  struct timeval timeout;
  fd_set set;
  if (inq->fd < 0)
  {
    errno = -ENOTSOCK;
    return -1;
  }
  nfds = inq->fd + 1;
  FD_ZERO(&set);
  FD_SET(inq->fd, &set);
  timeout.tv_sec = timeout_usec / (1000*1000);
  timeout.tv_usec = timeout_usec % (1000*1000);
  return select(nfds, &set, NULL, NULL, &timeout);
}

static void
ldp_in_queue_deallocate_some_socket(struct ldp_in_queue *inq,
                                    struct ldp_packet *pkts, int num)
{
  int i;
  int new_end;
  struct ldp_in_queue_socket *insock;
  insock = CONTAINER_OF(inq, struct ldp_in_queue_socket, q);
  if (num <= 0)
  {
    return;
  }
  new_end = insock->buf_end;
  for (i = 0; i < num; i++)
  {
    insock->bufs[new_end] = pkts[i].data;
    new_end++;
    if (new_end >= insock->num_bufs)
    {
      new_end = 0;
    }
  }
  insock->buf_end = new_end;
}

static int ldp_in_queue_nextpkts_socket(struct ldp_in_queue *inq,
                                        struct ldp_packet *pkts, int num)
{
  int i, j, k, l;
  int fd = inq->fd;
  int ret;
  int amnt_free;
  struct ldp_in_queue_socket *insock;
  insock = CONTAINER_OF(inq, struct ldp_in_queue_socket, q);

  // sz = 4
  //       X
  // 0 1 2 3
  // ^     ^


  // FIXME this calculation needs to be checked
  amnt_free = insock->buf_end - insock->buf_start - 1;
  if (amnt_free < 0)
  {
    amnt_free += insock->num_bufs;
  }
  if (amnt_free == 0)
  {
    return 0;
  }

  if (num > amnt_free)
  {
    num = amnt_free;
  }

  struct sockaddr_ll names[num];
  struct mmsghdr msgs[num];
  struct iovec iovecs[num][1];
  struct ldp_packet missed[num];

  memset(msgs, 0, sizeof(msgs));
  memset(iovecs, 0, sizeof(iovecs));

  j = insock->buf_start;
  for (i = 0; i < num; i++)
  {
    msgs[i].msg_hdr.msg_iovlen = 1;
    msgs[i].msg_hdr.msg_iov = iovecs[i];
    msgs[i].msg_hdr.msg_namelen = sizeof(names[i]);
    msgs[i].msg_hdr.msg_name = &names[i];
    if (j >= insock->num_bufs)
    {
      j = 0;
    }
    iovecs[i][0].iov_base = insock->bufs[j++];
    iovecs[i][0].iov_len = insock->max_sz;
  }
  ret = recvmmsg(fd, msgs, num, MSG_DONTWAIT, NULL);
  j = 0;
  k = insock->buf_start;
  l = 0;
  // FIXME what if all packets are outgoing?
  for (i = 0; i < ret; i++)
  {
    if (names[i].sll_pkttype == PACKET_OUTGOING)
    {
      missed[l].data = iovecs[i][0].iov_base;
      missed[l].sz = msgs[i].msg_len;
      k++;
      if (k >= insock->num_bufs)
      {
        k = 0;
      }
      l++;
      continue;
    }
    pkts[j].data = iovecs[i][0].iov_base;
    pkts[j].sz = msgs[i].msg_len;
    k++;
    if (k >= insock->num_bufs)
    {
      k = 0;
    }
    j++;
  }
  insock->buf_start = k;
  ldp_in_queue_deallocate_some_socket(inq, missed, l);
  return j;
}

static int ldp_out_queue_inject_socket(struct ldp_out_queue *outq,
                                       struct ldp_packet *pkts, int num)
{
  int i;
  int fd = outq->fd;
  int ret;

  if (num <= 0)
  {
    return 0;
  }

  struct mmsghdr msgs[num];
  struct iovec iovecs[num][1];

  memset(msgs, 0, sizeof(msgs));
  memset(iovecs, 0, sizeof(iovecs));

  for (i = 0; i < num; i++)
  {
    msgs[i].msg_hdr.msg_iovlen = 1;
    msgs[i].msg_hdr.msg_iov = iovecs[i];
    iovecs[i][0].iov_base = pkts[i].data;
    iovecs[i][0].iov_len = pkts[i].sz;
  }
  ret = sendmmsg(fd, msgs, num, 0);
  return ret;
}

static int ldp_out_queue_inject_chunk_socket(struct ldp_out_queue *outq,
                                             struct ldp_chunkpacket *pkts,
                                             int num)
{
  int i;
  int fd = outq->fd;
  int ret;

  if (num <= 0)
  {
    return 0;
  }

  struct mmsghdr msgs[num];

  memset(msgs, 0, sizeof(msgs));

  for (i = 0; i < num; i++)
  {
    msgs[i].msg_hdr.msg_iovlen = pkts[i].iovlen;
    msgs[i].msg_hdr.msg_iov = pkts[i].iov;
  }
  ret = sendmmsg(fd, msgs, num, 0);
  return ret;
}

static struct ldp_interface *
ldp_interface_open_socket(const char *name, int numinq, int numoutq,
                          const struct ldp_interface_settings *settings)
{
  struct ldp_interface *intf = NULL;
  struct ldp_in_queue **inqs = NULL;
  struct ldp_out_queue **outqs = NULL;
  struct ldp_in_queue_socket *insock = NULL;
  struct ldp_out_queue_socket *outsock = NULL;
  struct sockaddr_ll sockaddr_ll;
  int i;
  struct ifreq ifr;
  int ifindex;
  int mtu;
  int errnosave;

  if (numinq != 1 || numoutq != 1)
  {
    errno = ECHRNG;
    goto err;
  }
  intf = malloc(sizeof(*intf));
  if (intf == NULL)
  {
    errno = ENOMEM;
    goto err;
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
    errno = ENOMEM;
    goto err;
  }
  outqs = malloc(numoutq*sizeof(*outqs));
  if (outqs == NULL)
  {
    errno = ENOMEM;
    goto err;
  }


  insock = malloc(sizeof(*insock));
  if (insock == NULL)
  {
    errno = ENOMEM;
    goto err;
  }
  insock->bufs = NULL;
  insock->q.fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (insock->q.fd < 0)
  {
    // errno already set
    goto err;
  }

  memset(&ifr, 0, sizeof(ifr));
  if (settings && settings->mtu_set)
  {
    ifr.ifr_mtu = settings->mtu;
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name);
    if (ioctl(insock->q.fd, SIOCSIFMTU, &ifr) != 0)
    {
      // errno already set
      goto err;
    }
  }

  memset(&ifr, 0, sizeof(ifr));
  snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name);
  if (ioctl(insock->q.fd, SIOCGIFMTU, &ifr) != 0)
  {
    // errno already set
    goto err;
  }
  mtu = ifr.ifr_mtu;
  memset(&ifr, 0, sizeof(ifr));
  snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name);
  if (ioctl(insock->q.fd, SIOCGIFINDEX, &ifr) != 0)
  {
    // errno already set
    goto err;
  }
  ifindex = ifr.ifr_ifindex;
  memset(&sockaddr_ll, 0, sizeof(sockaddr_ll));
  sockaddr_ll.sll_family = AF_PACKET;
  sockaddr_ll.sll_ifindex = ifindex;
  sockaddr_ll.sll_protocol = htons(ETH_P_ALL);
  if (bind(insock->q.fd,
           (struct sockaddr*)&sockaddr_ll, sizeof(sockaddr_ll)) < 0)
  {
    // errno already set
    goto err;
  }

  outsock = malloc(sizeof(*outsock));
  if (outsock == NULL)
  {
    errno = ENOMEM;
    goto err;
  }
  outsock->q.fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (outsock->q.fd < 0)
  {
    // errno already set
    goto err;
  }

#ifdef PACKET_QDISC_BYPASS
  int one = 1;
  one = 1;
  setsockopt(outsock->q.fd, SOL_PACKET, PACKET_QDISC_BYPASS, &one, sizeof(one));
#endif

  memset(&sockaddr_ll, 0, sizeof(sockaddr_ll));
  sockaddr_ll.sll_family = AF_PACKET;
  sockaddr_ll.sll_ifindex = ifindex;
  sockaddr_ll.sll_protocol = htons(ETH_P_ALL);
  if (bind(outsock->q.fd,
           (struct sockaddr*)&sockaddr_ll, sizeof(sockaddr_ll)) < 0)
  {
    // errno already set
    goto err;
  }

  insock->q.nextpkts = ldp_in_queue_nextpkts_socket;
  insock->q.nextpkts_ts = NULL;
  insock->q.poll = ldp_in_queue_poll;
  insock->q.eof = NULL;
  insock->q.close = ldp_in_queue_close_socket;
  insock->q.deallocate_all = NULL;
  insock->q.deallocate_some = ldp_in_queue_deallocate_some_socket;
  insock->q.ring_size = ldp_in_queue_ring_size_socket;

  outsock->q.inject = ldp_out_queue_inject_socket;
  outsock->q.inject_dealloc = NULL;
  outsock->q.inject_chunk = ldp_out_queue_inject_chunk_socket;
  outsock->q.txsync = ldp_out_queue_txsync_socket;
  outsock->q.close = ldp_out_queue_close_socket;

  inqs[0] = &insock->q;
  outqs[0] = &outsock->q;
  
  intf->num_inq = numinq;
  intf->num_outq = numoutq;
  intf->inq = inqs;
  intf->outq = outqs;
  snprintf(intf->name, sizeof(intf->name), "%s", name);

  insock->max_sz = mtu + 14;
  insock->num_bufs = ldp_config_get_global()->socket_num_bufs;
  insock->buf_start = 0;
  insock->buf_end = 0;
  insock->bufs = malloc(insock->num_bufs*sizeof(*insock->bufs));
  if (insock->bufs == NULL)
  {
    errno = ENOMEM;
    goto err;
  }
  for (i = 0; i < insock->num_bufs; i++)
  {
    insock->bufs[i] = NULL;
  }
  for (i = 0; i < insock->num_bufs; i++)
  {
    insock->bufs[i] = malloc(insock->max_sz);
    if (insock->bufs[i] == NULL)
    {
      errno = ENOMEM;
      goto err;
    }
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

err:
  errnosave = errno;
  if (insock)
  {
    if (insock->bufs)
    {
      for (i = 0; i < insock->num_bufs; i++)
      {
        free(insock->bufs[i]);
      }
    }
    free(insock->bufs);
  }
  if (outsock && outsock->q.fd >= 0)
  {
    close(outsock->q.fd);
  }
  free(outsock);
  if (insock && insock->q.fd >= 0)
  {
    close(insock->q.fd);
  }
  free(insock);
  free(inqs);
  free(outqs);
  free(intf);
  errno = errnosave;
  return NULL;
}

void ldp_interface_close(struct ldp_interface *intf)
{
  int numinq = intf->num_inq;
  int numoutq = intf->num_outq;
  struct ldp_in_queue **inqs = intf->inq;
  struct ldp_out_queue **outqs = intf->outq;
  int i;

  for (i = 0; i < numinq; i++)
  {
    inqs[i]->close(inqs[i]);
  }
  for (i = 0; i < numoutq; i++)
  {
    outqs[i]->close(outqs[i]);
  }
  free(inqs);
  free(outqs);
  free(intf);
}

struct ldp_interface *
ldp_interface_open_2(const char *name, int numinq, int numoutq,
                     const struct ldp_interface_settings *settings)
{
  long portid;
  char *endptr;
  if (numinq < 0 || numoutq < 0 || numinq > 1024*1024 || numoutq > 1024*1024)
  {
    abort();
  }
  portid = strtol(name, &endptr, 10);
  if (*name != '\0' && *endptr == '\0' && portid >= 0 && portid <= INT_MAX)
  {
#if WITH_DPDK
    return ldp_interface_open_dpdk(name, numinq, numoutq, settings);
#else
    return NULL;
#endif
  }
  if (strncmp(name, "pcap:", 5) == 0)
  {
    return ldp_interface_open_pcap(name, numinq, numoutq, settings);
  }
  else if (strncmp(name, "null:", 5) == 0)
  {
    return ldp_interface_open_null(name, numinq, numoutq, settings);
  }
  else if (strncmp(name, "odp:", 4) == 0)
  {
#if WITH_ODP
    return ldp_interface_open_odp(name+4, numinq, numoutq, settings);
#else
    return NULL;
#endif
  }
  else if (strncmp(name, "netmap:", 7) == 0)
  {
#if WITH_NETMAP
    return ldp_interface_open_netmap(name, numinq, numoutq, settings);
#else
    return NULL;
#endif
  }
  else if (strncmp(name, "vale", 4) == 0)
  {
#if WITH_NETMAP
    return ldp_interface_open_netmap(name, numinq, numoutq, settings);
#else
    return NULL;
#endif
  }
  return ldp_interface_open_socket(name, numinq, numoutq, settings);
}
