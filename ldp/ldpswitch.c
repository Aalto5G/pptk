#define _GNU_SOURCE
#include <pthread.h>
#include "hashtable.h"
#include "hashlist.h"
#include "iphdr.h"
#include "containerof.h"
#include "siphash.h"
#include "hashseed.h"
#include "ipcksum.h"
#include <sys/poll.h>
#include "time64.h"
#include "ldp.h"

struct ldp_interface **intfs;
pthread_t *thrs;
struct thrctx *thrctxs;
int num_intfs;
int num_thrs;

struct hash_table table;

struct thrctx {
  int id;
};

struct entry {
  struct hash_list_node e;
  char mac[6];
  int port;
};

static inline uint32_t mac_hash(const char mac[6])
{
  return siphash_buf(hash_seed_get(), mac, 6);
}

static inline uint32_t hash(struct entry *e)
{
  return mac_hash(e->mac);
}

static uint32_t mac_hash_fn(struct hash_list_node *e, void *userdata)
{
  return hash(CONTAINER_OF(e, struct entry, e));
}

static int port_get(const char mac[6])
{
  uint32_t hashval = mac_hash(mac);
  struct hash_list_node *x;
  int port = -1;

  hash_table_lock_bucket(&table, hashval);
  HASH_TABLE_FOR_EACH_POSSIBLE(&table, x, hashval)
  {
    struct entry *e = CONTAINER_OF(x, struct entry, e);
    if (memcmp(e->mac, mac, 6) == 0)
    {
      port = e->port;
      break;
    }
  }
  hash_table_unlock_bucket(&table, hashval);
  return port;
}

static void port_put(const char mac[6], int port)
{
  uint32_t hashval = mac_hash(mac);
  struct hash_list_node *x;
  struct entry *e2 = NULL;

  hash_table_lock_bucket(&table, hashval);
  HASH_TABLE_FOR_EACH_POSSIBLE(&table, x, hashval)
  {
    struct entry *e = CONTAINER_OF(x, struct entry, e);
    if (memcmp(e->mac, mac, 6) == 0)
    {
      e2 = e;
      e2->port = port;
      break;
    }
  }
  if (e2 == NULL)
  {
    e2 = malloc(sizeof(*e2));
    if (e2 == NULL)
    {
      goto out;
    }
    memcpy(e2->mac, mac, 6);
    e2->port = port;
    hash_table_add_nogrow_already_bucket_locked(&table, &e2->e, hashval);
  }

out:
  hash_table_unlock_bucket(&table, hashval);
}

static void *thrfn(void *arg)
{
  struct thrctx *thrctx = arg;
  int id = thrctx->id;
  struct pollfd *pfds_template;
  struct pollfd *pfds;
  int do_poll = 1;
  int i, j, k, num;
  struct ldp_packet pkt_tbl[256];
  struct ldp_packet pktout_tbl[num_intfs][256];
  int numout[num_intfs];

  pfds_template = malloc(sizeof(*pfds_template)*num_intfs);
  if (pfds_template == NULL)
  {
    printf("out of memory\n");
    exit(1);
  }
  pfds = malloc(sizeof(*pfds)*num_intfs);
  if (pfds == NULL)
  {
    printf("out of memory\n");
    exit(1);
  }
  for (k = 0; k < num_intfs; k++)
  {
    numout[k] = 0;
  }

  for (i = 0; i < num_intfs; i++)
  {
    pfds_template[i].fd = intfs[i]->inq[id]->fd;
    pfds_template[i].events = POLLIN;
    if (pfds_template[i].fd < 0)
    {
      do_poll = 0;
      break;
    }
  }

  for (;;)
  {
    if (do_poll)
    {
      memcpy(pfds, pfds_template, sizeof(*pfds)*num_intfs);
      poll(pfds, num_intfs, 5);
    }
    for (i = 0; i < num_intfs; i++)
    {
      num = ldp_in_nextpkts(intfs[i]->inq[id], pkt_tbl, sizeof(pkt_tbl)/sizeof(*pkt_tbl));
      for (j = 0; j < num; j++)
      {
        int port;
        if (pkt_tbl[j].sz >= 12)
        {
          char *src = ether_src(pkt_tbl[j].data);
          if (!(src[0] & 1))
          {
            port_put(src, i);
          }
          port = port_get(ether_dst(pkt_tbl[j].data));
        }
        else
        {
          port = -1;
        }
        if (port == -1)
        {
          for (k = 0; k < num_intfs; k++)
          {
            if (k != i)
            {
              pktout_tbl[k][numout[k]++] = pkt_tbl[j];
            }
          }
        }
        else
        {
          pktout_tbl[port][numout[port]++] = pkt_tbl[j];
        }
      }
      for (k = 0; k < num_intfs; k++)
      {
        if (numout[k])
        {
          ldp_out_inject(intfs[k]->outq[id], pktout_tbl[k], numout[k]);
          numout[k] = 0;
        }
      }
      ldp_in_deallocate_some(intfs[i]->inq[id], pkt_tbl, num);
    }
  }
}

int main(int argc, char **argv)
{
  int i;
  int ret;

  hash_seed_init();

  setlinebuf(stdout);

  if (argc < 4)
  {
    printf("usage: %s 3 vale0:1 vale1:1 ...\n", argv[0]);
    exit(1);
  }

  num_intfs = argc - 2;
  intfs = malloc(num_intfs*sizeof(*intfs));
  if (intfs == NULL)
  {
    printf("out of memory\n");
    exit(1);
  }
  num_thrs = atoi(argv[1]);
  if (num_thrs < 1 || num_thrs > 1024)
  {
    printf("invalid thread count %s\n", argv[1]);
    exit(1);
  }
  if (num_thrs > 1)
  {
    ret = hash_table_init_locked(&table, 8192, mac_hash_fn, NULL, 1);
  }
  else
  {
    ret = hash_table_init(&table, 8192, mac_hash_fn, NULL);
  }
  if (ret != 0)
  {
    printf("out of memory\n");
    exit(1);
  }
  thrs = malloc(num_thrs*sizeof(*thrs));
  if (thrs == NULL)
  {
    printf("out of memory\n");
    exit(1);
  }
  thrctxs = malloc(num_thrs*sizeof(*thrctxs));
  if (thrctxs == NULL)
  {
    printf("out of memory\n");
    exit(1);
  }

  for (i = 0; i < num_intfs; i++)
  {
    intfs[i] = ldp_interface_open(argv[2+i], num_thrs, num_thrs);
  }

  for (i = 0; i < num_thrs; i++)
  {
    thrctxs[i].id = i;
    pthread_create(&thrs[i], NULL, thrfn, &thrctxs[i]);
  }
  for (i = 0; i < num_thrs; i++)
  {
    pthread_join(thrs[i], NULL);
  }

  return 0;
}
