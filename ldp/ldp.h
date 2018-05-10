#ifndef _LDP_H_
#define _LDP_H_

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <net/if.h>
#include <sys/uio.h>
#include "time64.h"

struct ldp_config {
  int netmap_nr_rx_slots; // 0: netmap default (our default may be different)
  int netmap_nr_tx_slots; // 0: netmap default (our default may be different)
  int dpdk_pool_num;
  int dpdk_pool_cache_num;
  int dpdk_pool_data_room;
  uint16_t dpdk_nb_rxd;
  uint16_t dpdk_nb_txd;
  int socket_num_bufs;
  int odp_num_pkt;
  int odp_pkt_len;
  int pcap_num_bufs;
};

struct ldp_config *ldp_config_get_global(void);

// Initialize config object
void ldp_config_init(struct ldp_config *config);

/*
 * Must be called before calling any other LDP function, if default is not ok
 *
 * Usage:
 *   struct ldp_config conf = {};
 *   ldp_config_init(&conf);
 *   conf.foo = bar;
 *   ...
 *   ldp_config_set(&conf);
 *
 */
void ldp_config_set(struct ldp_config *config);

struct ldp_interface_settings {
  int mtu_set;
  uint16_t mtu;
  int mac_addr_set;
  char mac[6];
  int promisc_set;
  int promisc_on;
  int allmulti_set;
  int allmulti_on;
};

struct ldp_interface {
  int num_inq;
  struct ldp_in_queue **inq;
  int num_outq;
  struct ldp_out_queue **outq;
  char name[IF_NAMESIZE];
  int (*mac_addr)(struct ldp_interface *intf, void *mac);
  int (*mac_addr_set)(struct ldp_interface *intf, const void *mac);
  int (*promisc_mode_get)(struct ldp_interface *intf);
  int (*promisc_mode_set)(struct ldp_interface *intf, int on);
  int (*allmulti_get)(struct ldp_interface *intf);
  int (*allmulti_set)(struct ldp_interface *intf, int on);
  int (*link_wait)(struct ldp_interface *intf);
  int (*link_status)(struct ldp_interface *intf);
};

struct ldp_packet {
  void *data;
  uint32_t sz;
  union {
    uint32_t ancillary;
    uint64_t ancillary64;
    size_t ancillarysz;
    void *ancillaryptr;
    char ancillarydata[sizeof(void*)];
  };
};

struct ldp_chunkpacket {
  struct iovec *iov;
  size_t iovlen;
};

struct ldp_in_queue {
  int fd;
  int (*nextpkts)(struct ldp_in_queue *inq,
                  struct ldp_packet *pkts, int num);
  int (*nextpkts_ts)(struct ldp_in_queue *inq,
                     struct ldp_packet *pkts, int num, uint64_t *time64);
  int (*poll)(struct ldp_in_queue *inq, uint64_t timeout_usec);
  int (*eof)(struct ldp_in_queue *inq);
  void (*close)(struct ldp_in_queue *inq);
  void (*deallocate_all)(struct ldp_in_queue *inq);
  void (*deallocate_some)(struct ldp_in_queue *inq,
                          struct ldp_packet *pkts, int num);
  uint32_t (*ring_size)(struct ldp_in_queue *inq);
};

struct ldp_out_queue {
  int fd;
  int (*inject)(struct ldp_out_queue *outq, struct ldp_packet *packets,
                int num);
  int (*inject_chunk)(struct ldp_out_queue *outq, struct ldp_chunkpacket *pkts,
                      int num);
  int (*inject_dealloc)(struct ldp_in_queue *inq, struct ldp_out_queue *outq,
                        struct ldp_packet *packets, int num);
  int (*txsync)(struct ldp_out_queue *outq);
  void (*close)(struct ldp_out_queue *outq);
};

static inline int ldp_inout_inject_dealloc(struct ldp_in_queue *inq,
                                           struct ldp_out_queue *outq,
                                           struct ldp_packet *packets, int num)
{
  int result;
  int num_sent;
  if (outq->inject_dealloc)
  {
    return outq->inject_dealloc(inq, outq, packets, num);
  }
  result = outq->inject(outq, packets, num);
  num_sent = result;
  if (num_sent < 0)
  {
    num_sent = 0;
  }
  if (num_sent > 0 && inq->deallocate_some)
  {
    inq->deallocate_some(inq, packets, num_sent);
  }
  return result;
}

static inline uint32_t ldp_in_ring_size(struct ldp_in_queue *inq)
{
  return inq->ring_size(inq);
}

static inline int ldp_in_nextpkts(struct ldp_in_queue *inq,
                                  struct ldp_packet *pkts, int num)
{
  return inq->nextpkts(inq, pkts, num);
}

static inline int ldp_in_nextpkts_ts(struct ldp_in_queue *inq,
                                     struct ldp_packet *pkts, int num,
                                     uint64_t *ts)
{
  int ret;
  if (inq->nextpkts_ts)
  {
    return inq->nextpkts_ts(inq, pkts, num, ts);
  }
  ret = inq->nextpkts(inq, pkts, num);
  if (ts)
  {
    *ts = gettime64();
  }
  return ret;
}

static inline int ldp_in_poll(struct ldp_in_queue *inq, uint64_t timeout_usec)
{
  int result;
#if 0 // Not done anymore
  if (inq->deallocate_all)
  {
    inq->deallocate_all(inq);
  }
#endif
  result = inq->poll(inq, timeout_usec);
  return result;
}

static inline void ldp_in_deallocate_all(struct ldp_in_queue *inq)
{
  abort(); // Not supported anymore
#if 0
  if (inq->deallocate_all)
  {
    inq->deallocate_all(inq);
  }
#endif
}

static inline void ldp_in_deallocate_some(struct ldp_in_queue *inq,
                                          struct ldp_packet *pkts, int num)
{
  if (num <= 0)
  {
    return;
  }
  if (inq->deallocate_some)
  {
    inq->deallocate_some(inq, pkts, num);
  }
}

static inline int ldp_in_eof(struct ldp_in_queue *inq)
{
  if (inq->eof == NULL)
  {
    return 0;
  }
  return inq->eof(inq);
}

static inline int ldp_out_inject(struct ldp_out_queue *outq,
                                 struct ldp_packet *pkts, int num)
{
  return outq->inject(outq, pkts, num);
}

static inline int ldp_out_inject_chunk(struct ldp_out_queue *outq,
                                       struct ldp_chunkpacket *pkts, int num)
{
  return outq->inject_chunk(outq, pkts, num);
}

static inline int ldp_out_txsync(struct ldp_out_queue *outq)
{
  return outq->txsync(outq);
}

struct ldp_interface *
ldp_interface_open_2(const char *name, int numinq, int numoutq,
                     const struct ldp_interface_settings *settings);

static inline struct ldp_interface *
ldp_interface_open(const char *name, int numinq, int numoutq)
{
  return ldp_interface_open_2(name, numinq, numoutq, NULL);
}

void ldp_interface_close(struct ldp_interface *intf);

int ldp_in_queue_poll(struct ldp_in_queue *inq, uint64_t timeout_usec);

#endif
