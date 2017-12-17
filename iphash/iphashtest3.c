#include "hashtable.h"
#include "murmur.h"
#include "timerlink.h"
#include "containerof.h"
#include <sys/time.h>
#include "time64.h"

#define INITIAL_TOKENS 2000
#define HASH_SIZE 0x20000
#define TIMER_PERIOD (1000*1000)
#define TIMER_ADD 400


struct ip_hash_entry3 { // 4 bytes total
  uint32_t tokens; // 4 bytes
};

struct ip_hash3 {
  struct ip_hash_entry3 *entries;
};

void ip_hash_timer_fn(struct timer_link *timer, struct timer_linkheap *heap, void *ud);

static void ip_hash_init(struct ip_hash3 *hash)
{
  size_t i;
  hash->entries = malloc(HASH_SIZE*sizeof(*hash->entries));
  for (i = 0; i < HASH_SIZE; i++)
  {
    struct ip_hash_entry3 *e = &hash->entries[i];
    e->tokens = INITIAL_TOKENS;
  }
}

static int ip_permitted(
  uint32_t src_ip, struct timer_linkheap *heap, struct ip_hash3 *hash)
{
  uint32_t class_c = src_ip&0xFFFFFF00U;
  struct ip_hash_entry3 *e = NULL;
  uint32_t hashval = murmur32(0x12345678U, class_c)&(HASH_SIZE - 1);
  e = &hash->entries[hashval];
  if (e->tokens == 0)
  {
    return 0;
  }
  e->tokens--;
  return 1;
}

struct batch_timer_userdata {
  struct ip_hash3 *hash;
  size_t start;
  size_t end;
};

static void batch_timer_fn(
  struct timer_link *timer, struct timer_linkheap *heap, void *ud)
{
  struct batch_timer_userdata *args = ud;
  size_t i;
  struct ip_hash_entry3 *e;
  for (i = args->start; i < args->end; i++)
  {
    e = &args->hash->entries[i];
    e->tokens += TIMER_ADD;
    if (e->tokens >= INITIAL_TOKENS)
    {
      e->tokens = INITIAL_TOKENS;
    }
  }
  timer->time64 += TIMER_PERIOD;
  timer_linkheap_add(heap, timer);
}

int main(int argc, char **argv)
{
  struct timer_linkheap heap;
  struct ip_hash3 hash;
  uint32_t addr;
  size_t iter, inner;
  size_t timer_burst;
  struct timeval tv1, tv2;
  struct batch_timer_userdata timerud[HASH_SIZE/16384];
  struct timer_link timers[HASH_SIZE/16384];
  size_t timercnt;
  timer_linkheap_init(&heap);
  ip_hash_init(&hash);
  timercnt = sizeof(timers)/sizeof(*timers);
  for (iter = 0; iter < sizeof(timers)/sizeof(*timers); iter++)
  {
    timerud[iter].hash = &hash;
    timerud[iter].start = 16384*iter;
    timerud[iter].end = 16384*(iter+1);
    timers[iter].fn = batch_timer_fn;
    timers[iter].userdata = &timerud[iter];
    timers[iter].time64 = gettime64() + TIMER_PERIOD*iter/timercnt;
  }
  iter = 0;
  while (iter < 128*1024*1024)
  {
    timer_burst = 0;
    while (gettime64() >= timer_linkheap_next_expiry_time(&heap))
    {
      struct timer_link *timer = timer_linkheap_next_expiry_timer(&heap);
      timer_linkheap_remove(&heap, timer);
      timer->fn(timer, &heap, timer->userdata);
      timer_burst++;
    }
    if (timer_burst >= 5000)
    {
      printf("timer burst of over 5000: %zu\n", timer_burst);
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
  while (heap.size)
  {
    struct timer_link *timer = timer_linkheap_next_expiry_timer(&heap);
    timer_linkheap_remove(&heap, timer);
  }
  gettimeofday(&tv1, NULL);
  for (iter = 0; iter < HASH_SIZE; iter++)
  {
    hash.entries[iter].tokens += TIMER_ADD;
    if (hash.entries[iter].tokens > INITIAL_TOKENS)
    {
      hash.entries[iter].tokens = INITIAL_TOKENS;
    }
  }
  gettimeofday(&tv2, NULL);
  printf("%lu us\n", (long)((tv2.tv_sec - tv1.tv_sec)*1000*1000 + tv2.tv_usec - tv1.tv_usec));
  gettimeofday(&tv1, NULL);
  for (iter = 0; iter < 16384; iter++)
  {
    hash.entries[iter].tokens += TIMER_ADD;
    if (hash.entries[iter].tokens > INITIAL_TOKENS)
    {
      hash.entries[iter].tokens = INITIAL_TOKENS;
    }
  }
  gettimeofday(&tv2, NULL);
  printf("%lu us\n", (long)((tv2.tv_sec - tv1.tv_sec)*1000*1000 + tv2.tv_usec - tv1.tv_usec));
  free(hash.entries);
  timer_linkheap_free(&heap);
  return 0;
}
