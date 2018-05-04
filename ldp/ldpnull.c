#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "ldp.h"
#include "ldpnull.h"
#include "containerof.h"


struct ldp_in_queue_null {
  struct ldp_in_queue q;
  int pipe2;
};

struct ldp_out_queue_null {
  struct ldp_out_queue q;
  int pipe2;
};

static uint32_t ldp_in_queue_ring_size_null(struct ldp_in_queue *inq)
{
  return 1024; // XXX dummy
}

static void ldp_in_queue_close_null(struct ldp_in_queue *inq)
{
  struct ldp_in_queue_null *innullq;
  innullq = CONTAINER_OF(inq, struct ldp_in_queue_null, q);
  close(innullq->q.fd);
  close(innullq->pipe2);
  free(innullq);
}

static void ldp_out_queue_close_null(struct ldp_out_queue *outq)
{
  struct ldp_out_queue_null *outnullq;
  outnullq = CONTAINER_OF(outq, struct ldp_out_queue_null, q);
  close(outnullq->q.fd);
  close(outnullq->pipe2);
  free(outnullq);
}


static int ldp_in_queue_nextpkts_null(struct ldp_in_queue *inq,
                                      struct ldp_packet *pkts, int num)
{
  return 0;
}

static int ldp_out_queue_inject_null(struct ldp_out_queue *outq,
                                     struct ldp_packet *packets, int num)
{
  return num;
}

static int ldp_out_queue_inject_chunk_null(struct ldp_out_queue *outq,
                                           struct ldp_chunkpacket *packets,
                                           int num)
{
  return num;
}

static int ldp_out_queue_txsync_null(struct ldp_out_queue *outq)
{
  return 0;
}

struct ldp_interface *
ldp_interface_open_null(const char *name, int numinq, int numoutq,
                        const struct ldp_interface_settings *settings)
{
  struct ldp_interface *intf = NULL;
  struct ldp_in_queue **inqs = NULL;
  struct ldp_out_queue **outqs = NULL;
  int i;
  int pipefd[2];
  if (numinq < 0 || numoutq < 0 || numinq > 1024*1024 || numoutq > 1024*1024)
  {
    abort();
  }
  intf = malloc(sizeof(*intf));
  if (intf == NULL)
  {
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
    goto err;
  }
  for (i = 0; i < numinq; i++)
  {
    inqs[i] = NULL;
  }
  outqs = malloc(numoutq*sizeof(*outqs));
  if (outqs == NULL)
  {
    goto err;
  }
  for (i = 0; i < numoutq; i++)
  {
    outqs[i] = NULL;
  }
  for (i = 0; i < numinq; i++)
  {
    struct ldp_in_queue_null *innullq;
    innullq = malloc(sizeof(*innullq));
    if (innullq == NULL)
    {
      goto err;
    }
    inqs[i] = &innullq->q;
    if (pipe(pipefd) != 0)
    {
      goto err;
    }
    innullq->q.fd = pipefd[0];
    innullq->pipe2 = pipefd[1];
    innullq->q.nextpkts = ldp_in_queue_nextpkts_null;
    innullq->q.nextpkts_ts = NULL;
    innullq->q.poll = ldp_in_queue_poll;
    innullq->q.eof = NULL;
    innullq->q.close = ldp_in_queue_close_null;
    innullq->q.deallocate_all = NULL;
    innullq->q.deallocate_some = NULL;
    innullq->q.ring_size = ldp_in_queue_ring_size_null;
  }
  for (i = 0; i < numoutq; i++)
  {
    struct ldp_out_queue_null *outnullq;
    outnullq = malloc(sizeof(*outnullq));
    if (outnullq == NULL)
    {
      goto err;
    }
    outqs[i] = &outnullq->q;
    if (pipe(pipefd) != 0)
    {
      goto err;
    }
    outnullq->pipe2 = pipefd[0];
    outnullq->q.fd = pipefd[1];
    outnullq->q.inject = ldp_out_queue_inject_null;
    outnullq->q.inject_chunk = ldp_out_queue_inject_chunk_null;
    outnullq->q.inject_dealloc = NULL;
    outnullq->q.txsync = ldp_out_queue_txsync_null;
    outnullq->q.close = ldp_out_queue_close_null;
  }
  intf->num_inq = numinq;
  intf->num_outq = numoutq;
  intf->inq = inqs;
  intf->outq = outqs;
  snprintf(intf->name, sizeof(intf->name), "%s", name);
  return intf;

err:
  if (inqs)
  {
    for (i = 0; i < numinq; i++)
    {
      if (inqs[i] != NULL)
      {
        struct ldp_in_queue_null *innmq;
        innmq = CONTAINER_OF(inqs[i], struct ldp_in_queue_null, q);
        free(innmq);
      }
    }
  }
  if (outqs)
  {
    for (i = 0; i < numoutq; i++)
    {
      if (outqs[i] != NULL)
      {
        struct ldp_out_queue_null *outnmq;
        outnmq = CONTAINER_OF(outqs[i], struct ldp_out_queue_null, q);
        free(outnmq);
      }
    }
  }
  free(inqs);
  free(outqs);
  free(intf);
  return NULL;
}
