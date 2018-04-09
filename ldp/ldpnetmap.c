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
#include "ldpnetmap.h"
#include "linkcommon.h"
#include "containerof.h"


struct ldp_in_queue_netmap {
  struct ldp_in_queue q;
  struct nm_desc *nmd;
};

struct ldp_out_queue_netmap {
  struct ldp_out_queue q;
  struct nm_desc *nmd;
};

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

struct ldp_interface *
ldp_interface_open_netmap(const char *name, int numinq, int numoutq,
                          const struct ldp_interface_settings *settings)
{
  char nmifnamebuf[1024] = {0};
  struct ldp_interface *intf;
  struct ldp_in_queue **inqs;
  struct ldp_out_queue **outqs;
  struct nmreq nmr;
  int i;
  int max;
  const char *devname = NULL;
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
    // nop
  }
  else
  {
    return NULL;
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
    struct ldp_in_queue_netmap *innmq;
    innmq = malloc(sizeof(*innmq));
    if (innmq == NULL)
    {
      abort(); // FIXME better error handling
    }
    inqs[i] = &innmq->q;
  }
  if (settings && settings->mtu_set && devname)
  {
    // Do this early in case we can't do it for a running interface
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
      abort(); // FIXME better error handling
    }
    if (ldp_set_mtu(sockfd, devname, settings->mtu) != 0)
    {
      abort(); // FIXME better error handling
    }
    close(sockfd);
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
