#ifndef _IPHASH_H_
#define _IPHASH_H_

#include "timerlink.h"
#include <sys/time.h>

struct ip_hash_entry {
  uint32_t tokens;
};

struct ip_hash_entry_small {
  uint16_t tokens;
};

struct batch_timer_userdata;

struct ip_hash {
  union {
    struct ip_hash_entry *entries;
    struct ip_hash_entry_small *entries_small;
  } u;
  struct timer_link *timers;
  struct batch_timer_userdata *timerud;
  uint32_t initial_tokens;
  uint32_t timer_period;
  uint32_t timer_add;
  uint32_t hash_size;
  uint32_t batch_size;
};

void ip_hash_init(struct ip_hash *hash, struct timer_linkheap *heap);

void ip_hash_free(struct ip_hash *hash, struct timer_linkheap *heap);

int ip_permitted(
  uint32_t src_ip, uint8_t bits, struct ip_hash *hash);

static inline int use_small(struct ip_hash *hash)
{
  return hash->initial_tokens <= 65535;
}

#endif
