#define NETMAP_WITH_LIBS
#define _GNU_SOURCE

#include <sys/poll.h>
#include <sys/socket.h>
#include <linux/ethtool.h>
#include <linux/if_packet.h>
#include <linux/sockios.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <arpa/inet.h>
#include "net/netmap_user.h"
#include "ldp.h"

#define CONTAINER_OF(ptr, type, member) \
  ((type*)(((char*)ptr) - (((char*)&(((type*)0)->member)) - ((char*)0))))

struct ldp_in_queue_netmap {
  struct ldp_in_queue q;
  struct nm_desc *nmd;
};

struct ldp_out_queue_netmap {
  struct ldp_out_queue q;
  struct nm_desc *nmd;
};

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

static void ldp_in_queue_close_netmap(struct ldp_in_queue *inq)
{
  struct ldp_in_queue_netmap *innmq;
  innmq = CONTAINER_OF(inq, struct ldp_in_queue_netmap, q);
  nm_close(innmq->nmd);
  free(innmq);
}

static void ldp_out_queue_close_netmap(struct ldp_out_queue *outq)
{
  struct ldp_out_queue_netmap *outnmq;
  outnmq = CONTAINER_OF(outq, struct ldp_out_queue_netmap, q);
  nm_close(outnmq->nmd);
  free(outnmq);
}


static inline void nm_ldp_inject(struct nm_desc *nmd, void *data, size_t sz)
{
  int i, j;
  for (i = 0; i < 3; i++)
  {
    for (j = 0; j < 3; j++)
    {
      if (nm_inject(nmd, data, sz) == 0)
      {
        struct pollfd pollfd;
        pollfd.fd = nmd->fd;
        pollfd.events = POLLOUT;
        poll(&pollfd, 1, 0);
      }
      else
      {
        return;
      }
    }
    ioctl(nmd->fd, NIOCTXSYNC, NULL);
  }
}

static int ldp_in_queue_poll(struct ldp_in_queue *inq, uint64_t timeout_usec)
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
  int i;
  int fd = inq->fd;
  int ret;
  struct ldp_in_queue_socket *insock;
  insock = CONTAINER_OF(inq, struct ldp_in_queue_socket, q);

  if (num > insock->num_bufs)
  {
    num = insock->num_bufs;
  }

  struct mmsghdr msgs[num];
  struct iovec iovecs[num][1];

  memset(msgs, 0, sizeof(msgs));
  memset(iovecs, 0, sizeof(iovecs));

  for (i = 0; i < num; i++)
  {
    msgs[i].msg_hdr.msg_iovlen = 1;
    msgs[i].msg_hdr.msg_iov = iovecs[i];
    iovecs[i][0].iov_base = insock->bufs[i];
    iovecs[i][0].iov_len = insock->max_sz;
  }
  ret = recvmmsg(fd, msgs, num, MSG_DONTWAIT, NULL);
  for (i = 0; i < ret; i++)
  {
    // FIXME bypass own packets
    pkts[i].data = insock->bufs[i];
    pkts[i].sz = msgs[i].msg_len;
  }
  return ret;
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

static int ldp_in_queue_nextpkts_netmap(struct ldp_in_queue *inq,
                                        struct ldp_packet *pkts, int num)
{
  int i;
  struct ldp_in_queue_netmap *innmq;
  innmq = CONTAINER_OF(inq, struct ldp_in_queue_netmap, q);
  for (i = 0; i < num; i++)
  {
    unsigned char *pkt;
    struct nm_pkthdr hdr;
    pkt = nm_nextpkt(innmq->nmd, &hdr);
    if (pkt == NULL)
    {
      break;
    }
    pkts[i].data = pkt;
    pkts[i].sz = hdr.len;
  }
  return i;
}

static int ldp_out_queue_inject_netmap(struct ldp_out_queue *outq,
                                       struct ldp_packet *packets, int num)
{
  int i;
  struct ldp_out_queue_netmap *outnmq;
  outnmq = CONTAINER_OF(outq, struct ldp_out_queue_netmap, q);
  for (i = 0; i < num; i++)
  {
    nm_ldp_inject(outnmq->nmd, packets[i].data, packets[i].sz);
  }
  return num;
}

static int ldp_out_queue_txsync_netmap(struct ldp_out_queue *outq)
{
  struct ldp_out_queue_netmap *outnmq;
  outnmq = CONTAINER_OF(outq, struct ldp_out_queue_netmap, q);
  if (ioctl(outnmq->nmd->fd, NIOCTXSYNC, NULL) == -1)
  {
    return -1;
  }
  return 0;
}

static int check_channels(const char *name, int numinq)
{
  int rx;
  struct ethtool_channels echannels = {};
  struct ifreq ifr = {};
  int fd;

  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0)
  {
    return 0;
  }
  echannels.cmd = ETHTOOL_GCHANNELS;
  snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name);
  ifr.ifr_data = (void*)&echannels; // cmd
  if (ioctl(fd, SIOCETHTOOL, &ifr) != 0)
  {
    if (errno == ENOTSUP && numinq == 1)
    {
      close(fd);
      return 1;
    }
    close(fd);
    return 0;
  }
  close(fd);

  rx = echannels.rx_count;
  if (rx <= 0)
  {
    rx = echannels.combined_count;
  }
  if (rx != numinq)
  {
    return 0;
  }
  return 1;
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

  if (numinq != 1 || numoutq != 1)
  {
    return NULL;
  }
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

  struct ifreq ifr;
  int ifindex;
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
  int one = 1;
  setsockopt(outsock->q.fd, SOL_PACKET, PACKET_QDISC_BYPASS, &one, sizeof(one));

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

  insock->max_sz = 1514;
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

struct ldp_interface *
ldp_interface_open(const char *name, int numinq, int numoutq)
{
  char nmifnamebuf[1024] = {0};
  struct ldp_interface *intf;
  struct ldp_in_queue **inqs;
  struct ldp_out_queue **outqs;
  struct nmreq nmr;
  int i;
  int max;
  const char *devname;
  if (numinq < 0 || numoutq < 0 || numinq > 1024*1024 || numoutq > 1024*1024)
  {
    abort();
  }
  if (strncmp(name, "netmap:", 7) == 0)
  {
    devname = name + 7;
    if (!check_channels(devname, numinq))
    {
      return NULL;
    }
  }
  else if (strncmp(name, "vale", 4) == 0)
  {
    
  }
  else
  {
    return ldp_interface_open_socket(name, numinq, numoutq);
  }
  max = numinq;
  if (max < numoutq)
  {
    max = numoutq;
  }
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
    struct ldp_in_queue_netmap *innmq;
    innmq = malloc(sizeof(*innmq));
    if (innmq == NULL)
    {
      abort(); // FIXME better error handling
    }
    inqs[i] = &innmq->q;
  }
  for (i = 0; i < numoutq; i++)
  {
    struct ldp_out_queue_netmap *outnmq;
    outnmq = malloc(sizeof(*outnmq));
    if (outnmq == NULL)
    {
      abort(); // FIXME better error handling
    }
    outqs[i] = &outnmq->q;
  }
  for (i = 0; i < numinq; i++)
  {
    struct ldp_in_queue_netmap *innmq;
    memset(&nmr, 0, sizeof(nmr));
    nmr.nr_tx_rings = max;
    nmr.nr_rx_rings = max;
    nmr.nr_flags = NR_REG_ONE_NIC;
    nmr.nr_ringid = i | NETMAP_NO_TX_POLL;
    nmr.nr_rx_slots = 256;
    nmr.nr_tx_slots = 64;
    snprintf(nmifnamebuf, sizeof(nmifnamebuf), "%s-%d", name, i);
    innmq = CONTAINER_OF(inqs[i], struct ldp_in_queue_netmap, q);
    innmq->nmd = nm_open(nmifnamebuf, &nmr, 0, NULL);
    innmq->q.nextpkts = ldp_in_queue_nextpkts_netmap;
    innmq->q.poll = ldp_in_queue_poll;
    innmq->q.close = ldp_in_queue_close_netmap;
    if (innmq->nmd == NULL)
    {
      while (--i >= 0)
      {
        innmq = CONTAINER_OF(inqs[i], struct ldp_in_queue_netmap, q);
        nm_close(innmq->nmd);
      }
      goto err;
    }
    innmq->q.fd = innmq->nmd->fd;
  }
  for (i = 0; i < numoutq; i++)
  {
    struct ldp_out_queue_netmap *outnmq;
    memset(&nmr, 0, sizeof(nmr));
    nmr.nr_tx_rings = max;
    nmr.nr_rx_rings = max;
    nmr.nr_flags = NR_REG_ONE_NIC;
    nmr.nr_ringid = i | NETMAP_NO_TX_POLL;
    nmr.nr_rx_slots = 256;
    nmr.nr_tx_slots = 64;
    snprintf(nmifnamebuf, sizeof(nmifnamebuf), "%s-%d", name, i);
    outnmq = CONTAINER_OF(outqs[i], struct ldp_out_queue_netmap, q);
    outnmq->nmd = nm_open(nmifnamebuf, &nmr, 0, NULL);
    outnmq->q.inject = ldp_out_queue_inject_netmap;
    outnmq->q.txsync = ldp_out_queue_txsync_netmap;
    outnmq->q.close = ldp_out_queue_close_netmap;
    if (outnmq->nmd == NULL)
    {
      while (--i >= 0)
      {
        outnmq = CONTAINER_OF(outqs[i], struct ldp_out_queue_netmap, q);
        nm_close(outnmq->nmd);
      }
      for (i = 0; i < numinq; i++)
      {
        struct ldp_in_queue_netmap *innmq;
        innmq = CONTAINER_OF(inqs[i], struct ldp_in_queue_netmap, q);
        nm_close(innmq->nmd);
      }
      goto err;
    }
    outnmq->q.fd = outnmq->nmd->fd;
  }
  intf->num_inq = numinq;
  intf->num_outq = numoutq;
  intf->inq = inqs;
  intf->outq = outqs;
  return intf;

err:
  for (i = 0; i < numinq; i++)
  {
    struct ldp_in_queue_netmap *innmq;
    innmq = CONTAINER_OF(inqs[i], struct ldp_in_queue_netmap, q);
    free(innmq);
  }
  for (i = 0; i < numoutq; i++)
  {
    struct ldp_out_queue_netmap *outnmq;
    outnmq = CONTAINER_OF(outqs[i], struct ldp_out_queue_netmap, q);
    free(outnmq);
  }
  free(inqs);
  free(outqs);
  free(intf);
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
