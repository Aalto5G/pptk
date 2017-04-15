#include "hashtable.h"
#include "siphash.h"
#include "timerlink.h"
#include "containerof.h"
#include "iphash.h"
#include <sys/time.h>

static inline uint64_t gettime64(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec*1000UL*1000UL + tv.tv_usec;
}

struct batch_timer_userdata {
  struct ip_hash *hash;
  size_t start;
  size_t end;
};

static void batch_timer_fn(
  struct timer_link *timer, struct timer_linkheap *heap, void *ud);

static inline int power_of_2(size_t x)
{
  return (x > 0) && ((x & (x-1)) == 0);
}

void ip_hash_init(struct ip_hash *hash, struct timer_linkheap *heap)
{
  size_t i;
  size_t timercnt = hash->hash_size/hash->batch_size;
  if (!power_of_2(hash->hash_size) || !power_of_2(hash->batch_size) ||
      hash->hash_size < hash->batch_size)
  {
    abort();
  }
  hash->timers = malloc(timercnt*sizeof(*hash->timers));
  hash->timerud = malloc(timercnt*sizeof(*hash->timerud));
  for (i = 0; i < timercnt; i++)
  {
    hash->timerud[i].hash = hash;
    hash->timerud[i].start = hash->batch_size*i;
    hash->timerud[i].end = hash->batch_size*(i+1);
    hash->timers[i].fn = batch_timer_fn;
    hash->timers[i].userdata = &hash->timerud[i];
    hash->timers[i].time64 = gettime64() + hash->timer_period*i/timercnt;
    timer_linkheap_add(heap, &hash->timers[i]);
  }
  
  if (use_small(hash))
  {
    hash->u.entries_small = malloc(hash->hash_size*sizeof(*hash->u.entries_small));
    for (i = 0; i < hash->hash_size; i++)
    {
      struct ip_hash_entry_small *e = &hash->u.entries_small[i];
      e->tokens = hash->initial_tokens;
    }
  }
  else
  {
    hash->u.entries = malloc(hash->hash_size*sizeof(*hash->u.entries));
    for (i = 0; i < hash->hash_size; i++)
    {
      struct ip_hash_entry *e = &hash->u.entries[i];
      e->tokens = hash->initial_tokens;
    }
  }
}

const char key[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};

int ip_permitted(
  uint32_t src_ip, struct ip_hash *hash)
{
  uint32_t class_c = src_ip&0xFFFFFF00U;
  uint32_t hashval = siphash64(key, class_c)&(hash->hash_size - 1);
  if (use_small(hash))
  {
    struct ip_hash_entry_small *e = NULL;
    e = &hash->u.entries_small[hashval];
    if (e->tokens == 0)
    {
      return 0;
    }
    e->tokens--;
    return 1;
  }
  else
  {
    struct ip_hash_entry *e = NULL;
    e = &hash->u.entries[hashval];
    if (e->tokens == 0)
    {
      return 0;
    }
    e->tokens--;
    return 1;
  }
}

static void batch_timer_fn(
  struct timer_link *timer, struct timer_linkheap *heap, void *ud)
{
  struct batch_timer_userdata *args = ud;
  size_t i;
  uint32_t tokens;
  uint32_t timer_add = args->hash->timer_add;
  uint32_t initial_tokens = args->hash->initial_tokens;
  if (use_small(args->hash))
  {
    struct ip_hash_entry_small *e;
    for (i = args->start; i < args->end; i++)
    {
      e = &args->hash->u.entries_small[i];
      tokens = e->tokens + timer_add;
      if (tokens >= initial_tokens)
      {
        tokens = initial_tokens;
      }
      e->tokens = tokens;
    }
  }
  else
  {
    struct ip_hash_entry *e;
    for (i = args->start; i < args->end; i++)
    {
      e = &args->hash->u.entries[i];
      tokens = e->tokens + timer_add;
      if (tokens >= initial_tokens)
      {
        tokens = initial_tokens;
      }
      e->tokens = tokens;
    }
  }
  timer->time64 += args->hash->timer_period;
  timer_linkheap_add(heap, timer);
}
