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
  char **bufs;
};

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

static int ldp_in_queue_nextpkts_socket(struct ldp_in_queue *inq,
                                        struct ldp_packet *pkts, int num)
{
  int i, j;
  int fd = inq->fd;
  int ret;
  struct ldp_in_queue_socket *insock;
  insock = CONTAINER_OF(inq, struct ldp_in_queue_socket, q);

  if (num > insock->num_bufs)
  {
    num = insock->num_bufs;
  }

  struct sockaddr_ll names[num];
  struct mmsghdr msgs[num];
  struct iovec iovecs[num][1];

  memset(msgs, 0, sizeof(msgs));
  memset(iovecs, 0, sizeof(iovecs));

  for (i = 0; i < num; i++)
  {
    msgs[i].msg_hdr.msg_iovlen = 1;
    msgs[i].msg_hdr.msg_iov = iovecs[i];
    msgs[i].msg_hdr.msg_namelen = sizeof(names[i]);
    msgs[i].msg_hdr.msg_name = &names[i];
    iovecs[i][0].iov_base = insock->bufs[i];
    iovecs[i][0].iov_len = insock->max_sz;
  }
  ret = recvmmsg(fd, msgs, num, MSG_DONTWAIT, NULL);
  j = 0;
  for (i = 0; i < ret; i++)
  {
    if (names[i].sll_pkttype == PACKET_OUTGOING)
    {
      continue;
    }
    pkts[j].data = insock->bufs[i];
    pkts[j].sz = msgs[i].msg_len;
    j++;
  }
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
ldp_interface_open_socket(const char *name, int numinq, int numoutq)
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
  intf->link_wait = NULL;
  intf->link_status = NULL;
  intf->mac_addr = NULL;
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
  insock->q.poll = ldp_in_queue_poll;
  insock->q.close = ldp_in_queue_close_socket;

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
  insock->num_bufs = 128;
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
ldp_interface_open(const char *name, int numinq, int numoutq)
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
    return ldp_interface_open_dpdk(name, numinq, numoutq);
#else
    return NULL;
#endif
  }
  if (strncmp(name, "null:", 5) == 0)
  {
    return ldp_interface_open_null(name, numinq, numoutq);
  }
  else if (strncmp(name, "odp:", 4) == 0)
  {
#if WITH_ODP
    return ldp_interface_open_odp(name+4, numinq, numoutq);
#else
    return NULL;
#endif
  }
  else if (strncmp(name, "netmap:", 7) == 0)
  {
#if WITH_NETMAP
    return ldp_interface_open_netmap(name, numinq, numoutq);
#else
    return NULL;
#endif
  }
  else if (strncmp(name, "vale", 4) == 0)
  {
#if WITH_NETMAP
    return ldp_interface_open_netmap(name, numinq, numoutq);
#else
    return NULL;
#endif
  }
  return ldp_interface_open_socket(name, numinq, numoutq);
}
