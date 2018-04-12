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

static void ldp_in_queue_deallocate_all_socket(struct ldp_in_queue *inq)
{
  struct ldp_in_queue_socket *insock;
  insock = CONTAINER_OF(inq, struct ldp_in_queue_socket, q);
  insock->buf_start = 0;
  insock->buf_end = 0;
}

static void
ldp_in_queue_deallocate_some_socket(struct ldp_in_queue *inq,
                                    struct ldp_packet *pkts, int num)
{
  struct ldp_in_queue_socket *insock;
  insock = CONTAINER_OF(inq, struct ldp_in_queue_socket, q);
  if (num <= 0)
  {
    return;
  }
  insock->buf_end = pkts[num-1].ancillary + 1;
  if (insock->buf_end >= insock->num_bufs)
  {
    insock->buf_end = 0;
  }
}

static int ldp_in_queue_nextpkts_socket(struct ldp_in_queue *inq,
                                        struct ldp_packet *pkts, int num)
{
  int i, j, k;
  int last_k;
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
  last_k = insock->buf_start;
  // FIXME what if all packets are outgoing?
  for (i = 0; i < ret; i++)
  {
    if (names[i].sll_pkttype == PACKET_OUTGOING)
    {
      k++;
      if (k >= insock->num_bufs)
      {
        k = 0;
      }
      continue;
    }
    pkts[j].data = iovecs[i][0].iov_base;
    pkts[j].sz = msgs[i].msg_len;
    pkts[j].ancillary = k++;
    if (k >= insock->num_bufs)
    {
      k = 0;
    }
    last_k = k;
    j++;
  }
  insock->buf_start = last_k;
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

static struct ldp_interface *
ldp_interface_open_socket(const char *name, int numinq, int numoutq,
                          const struct ldp_interface_settings *settings)
{
  struct ldp_interface *intf;
  struct ldp_in_queue **inqs;
  struct ldp_out_queue **outqs;
  struct ldp_in_queue_socket *insock;
  struct ldp_out_queue_socket *outsock;
  struct sockaddr_ll sockaddr_ll;
  int i;
  struct ifreq ifr;
  int ifindex;
  int mtu;

  if (numinq != 1 || numoutq != 1)
  {
    return NULL;
  }
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


  insock = malloc(sizeof(*insock));
  if (insock == NULL)
  {
    abort();
  }
  insock->q.fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (insock->q.fd < 0)
  {
    abort();
  }

  memset(&ifr, 0, sizeof(ifr));
  if (settings && settings->mtu_set)
  {
    ifr.ifr_mtu = settings->mtu;
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name);
    if (ioctl(insock->q.fd, SIOCSIFMTU, &ifr) != 0)
    {
      close(insock->q.fd);
      free(insock);
      free(inqs);
      free(outqs);
      free(intf);
      return NULL;
    }
  }

  memset(&ifr, 0, sizeof(ifr));
  snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name);
  if (ioctl(insock->q.fd, SIOCGIFMTU, &ifr) != 0)
  {
    close(insock->q.fd);
    free(insock);
    free(inqs);
    free(outqs);
    free(intf);
    return NULL;
  }
  mtu = ifr.ifr_mtu;
  memset(&ifr, 0, sizeof(ifr));
  snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name);
  if (ioctl(insock->q.fd, SIOCGIFINDEX, &ifr) != 0)
  {
    close(insock->q.fd);
    free(insock);
    free(inqs);
    free(outqs);
    free(intf);
    return NULL;
  }
  ifindex = ifr.ifr_ifindex;
  memset(&sockaddr_ll, 0, sizeof(sockaddr_ll));
  sockaddr_ll.sll_family = AF_PACKET;
  sockaddr_ll.sll_ifindex = ifindex;
  sockaddr_ll.sll_protocol = htons(ETH_P_ALL);
  if (bind(insock->q.fd,
           (struct sockaddr*)&sockaddr_ll, sizeof(sockaddr_ll)) < 0)
  {
    close(insock->q.fd);
    free(insock);
    free(inqs);
    free(outqs);
    free(intf);
    return NULL;
  }

  outsock = malloc(sizeof(*outsock));
  if (outsock == NULL)
  {
    abort();
  }
  outsock->q.fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (outsock->q.fd < 0)
  {
    abort();
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
    close(outsock->q.fd);
    free(outsock);
    close(insock->q.fd);
    free(insock);
    free(inqs);
    free(outqs);
    free(intf);
    return NULL;
  }

  insock->q.nextpkts = ldp_in_queue_nextpkts_socket;
  insock->q.nextpkts_ts = NULL;
  insock->q.poll = ldp_in_queue_poll;
  insock->q.eof = NULL;
  insock->q.close = ldp_in_queue_close_socket;
  insock->q.deallocate_all = ldp_in_queue_deallocate_all_socket;
  insock->q.deallocate_some = ldp_in_queue_deallocate_some_socket;
  insock->q.ring_size = ldp_in_queue_ring_size_socket;

  outsock->q.inject = ldp_out_queue_inject_socket;
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
  insock->num_bufs = 1024;
  insock->buf_start = 0;
  insock->buf_end = 0;
  insock->bufs = malloc(insock->num_bufs*sizeof(*insock->bufs));
  if (insock->bufs == NULL)
  {
    abort();
  }
  for (i = 0; i < insock->num_bufs; i++)
  {
    insock->bufs[i] = malloc(insock->max_sz);
    if (insock->bufs[i] == NULL)
    {
      abort();
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
