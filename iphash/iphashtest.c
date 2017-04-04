#include "hashtable.h"
#include "murmur.h"
#include "timerlink.h"
#include "containerof.h"
#include <sys/time.h>

#define INITIAL_TOKENS 1000
#define HASH_SIZE 0x20000
#define TIMER_PERIOD (2*1000*1000)
#define TIMER_ADD 400

struct ip_hash {
  struct hash_table table;
};

struct ip_hash_entry { // 56 bytes total + ~8 bytes allocator overhead = 64
  struct hash_list_node node; // 16 bytes
  uint32_t hashval; // 4 bytes
  uint32_t tokens; // 4 bytes
  struct timer_link timer; // 32 bytes
};

static uint32_t ip_hash_entry_hash_fn(struct hash_list_node *n, void *userdata)
{
  struct ip_hash_entry *e = CONTAINER_OF(n, struct ip_hash_entry, node);
  return e->hashval;
}

static void ip_hash_init(struct ip_hash *hash)
{
  hash_table_init(&hash->table, HASH_SIZE, ip_hash_entry_hash_fn, NULL);
}

static inline uint64_t gettime64(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec*1000UL*1000UL + tv.tv_usec;
}

static void ip_hash_timer_fn(
  struct timer_link *timer, struct timer_linkheap *heap, void *ud)
{
  //struct hash_table *table = ud;
  struct ip_hash_entry *e = CONTAINER_OF(timer, struct ip_hash_entry, timer);
  e->tokens += TIMER_ADD;
  if (e->tokens >= INITIAL_TOKENS)
  {
    e->tokens = INITIAL_TOKENS;
#if 0
    hash_table_delete(table, &e->node);
    free(e);
#endif
  }
  else
  {
    timer->time64 += TIMER_PERIOD;
    timer_linkheap_add(heap, timer);
  }
}

static int ip_permitted(
  uint32_t src_ip, struct timer_linkheap *heap, struct ip_hash *hash)
{
  uint32_t class_c = src_ip&0xFFFFFF00U;
  struct ip_hash_entry *e = NULL;
  struct hash_list_node *n;
  uint32_t hashval = murmur32(0x12345678U, class_c)&(HASH_SIZE - 1);
  HASH_TABLE_FOR_EACH_POSSIBLE(&hash->table, n, hashval)
  {
    e = CONTAINER_OF(n, struct ip_hash_entry, node);
    if (e->hashval == hashval)
    {
      break;
    }
    else
    {
      printf("hash collision\n");
    }
  }
  if (e == NULL)
  {
    e = malloc(sizeof(*e));
    if (e == NULL)
    {
      abort();
    }
    e->tokens = INITIAL_TOKENS;
    e->hashval = hashval;
    e->timer.time64 = gettime64() + TIMER_PERIOD;
    e->timer.fn = ip_hash_timer_fn;
    e->timer.userdata = &hash->table;
    timer_linkheap_add(heap, &e->timer);
    hash_table_add_nogrow(&hash->table, &e->node, hashval);
  }
  else if (e->tokens == INITIAL_TOKENS)
  {
    e->timer.time64 = gettime64() + TIMER_PERIOD;
    timer_linkheap_add(heap, &e->timer);
  }
  if (e->tokens == 0)
  {
    return 0;
  }
  e->tokens--;
  return 1;
}

int main(int argc, char **argv)
{
  struct timer_linkheap heap;
  struct ip_hash hash;
  uint32_t addr;
  size_t iter, inner;
  timer_linkheap_init(&heap);
  ip_hash_init(&hash);
  iter = 0;
  while (iter < 128*1024*1024)
  {
    while (gettime64() >= timer_linkheap_next_expiry_time(&heap))
    {
      struct timer_link *timer = timer_linkheap_next_expiry_timer(&heap);
      timer_linkheap_remove(&heap, timer);
      timer->fn(timer, &heap, timer->userdata);
    }
    for (inner = 0; inner < 64; inner++)
    {
      addr = rand();
      if (!ip_permitted(addr, &heap, &hash))
      {
        printf("not permitted at iter %zu\n", iter);
      }
      iter++;
      if ((iter & ((1<<20) - 1)) == 0)
      {
        printf("iter %zu\n", iter);
      }
    }
  }
  return 0;
}
