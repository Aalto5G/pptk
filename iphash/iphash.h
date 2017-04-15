#ifndef _IPHASH_H_
#define _IPHASH_H_

#include "timerlink.h"
#include <sys/time.h>

struct ip_hash_entry5 {
  uint32_t tokens;
};

struct ip_hash_entry5small {
  uint16_t tokens;
};

struct batch_timer_userdata;

struct ip_hash5 {
  union {
    struct ip_hash_entry5 *entries;
    struct ip_hash_entry5small *entries_small;
  } u;
  struct timer_link *timers;
  struct batch_timer_userdata *timerud;
  uint32_t initial_tokens;
  uint32_t timer_period;
  uint32_t timer_add;
  uint32_t hash_size;
  uint32_t batch_size;
};

void ip_hash_init(struct ip_hash5 *hash, struct timer_linkheap *heap);

int ip_permitted(
  uint32_t src_ip, struct ip_hash5 *hash);

static inline int use_small(struct ip_hash5 *hash)
{
  return hash->initial_tokens <= 65535;
}

#endif
