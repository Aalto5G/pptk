#ifndef _LDP_H_
#define _LDP_H_

#include <stddef.h>
#include <stdint.h>
#include <net/if.h>
#include "time64.h"

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
  int (*promisc_mode_set)(struct ldp_interface *intf, int on);
  int (*allmulti_set)(struct ldp_interface *intf, int on);
  int (*link_wait)(struct ldp_interface *intf);
  int (*link_status)(struct ldp_interface *intf);
};

struct ldp_packet {
  void *data;
  uint32_t sz;
  uint32_t ancillary;
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
};

struct ldp_out_queue {
  int fd;
  int (*inject)(struct ldp_out_queue *outq, struct ldp_packet *packets,
                int num);
  int (*txsync)(struct ldp_out_queue *outq);
  void (*close)(struct ldp_out_queue *outq);
};

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
  if (inq->deallocate_all)
  {
    inq->deallocate_all(inq);
  }
  result = inq->poll(inq, timeout_usec);
  return result;
}

// All packets must be deallocated before polling for new packets to arrive
static inline void ldp_in_deallocate_all(struct ldp_in_queue *inq)
{
  if (inq->deallocate_all)
  {
    inq->deallocate_all(inq);
  }
}

// Packets must be deallocated in the same order they were allocated
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
