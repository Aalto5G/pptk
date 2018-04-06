#ifndef _LDP_H_
#define _LDP_H_

#include <stddef.h>
#include <stdint.h>

struct ldp_interface {
  int num_inq;
  struct ldp_in_queue **inq;
  int num_outq;
  struct ldp_out_queue **outq;
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

struct ldp_interface *
ldp_interface_open(const char *name, int numinq, int numoutq);

void ldp_interface_close(struct ldp_interface *intf);

#endif
