#ifndef _LDP_H_
#define _LDP_H_

#include <stddef.h>
#include <stdint.h>
#include <net/if.h>

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
  size_t sz;
};

struct ldp_in_queue {
  int fd;
  int (*nextpkts)(struct ldp_in_queue *inq,
                  struct ldp_packet *pkts, int num);
  int (*poll)(struct ldp_in_queue *inq, uint64_t timeout_usec);
  void (*close)(struct ldp_in_queue *inq);
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

static inline int ldp_in_poll(struct ldp_in_queue *inq, uint64_t timeout_usec)
{
  return inq->poll(inq, timeout_usec);
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
