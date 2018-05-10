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

struct ip_hash_entry_tiny {
  uint8_t tokens;
};

struct batch_timer_userdata;

struct ip_hash {
  union {
    struct ip_hash_entry *entries;
    struct ip_hash_entry_small *entries_small;
    struct ip_hash_entry_tiny *entries_tiny;
  } u;
  struct timer_link *timers;
  struct batch_timer_userdata *timerud;
  uint32_t initial_tokens;
  uint32_t timer_period;
  uint32_t timer_add;
  uint32_t hash_size;
  uint32_t batch_size;
};

void ip_hash_init(struct ip_hash *hash, struct timer_linkheap *heap,
                  pthread_rwlock_t *lock);

void ip_hash_free(struct ip_hash *hash, struct timer_linkheap *heap);

int ip_permitted(
  uint32_t src_ip, uint8_t bits, struct ip_hash *hash);

int ipv6_permitted(
  const void *src_ip, uint8_t bits, struct ip_hash *hash);

void ip_increment_one(
  uint32_t src_ip, uint8_t bits, struct ip_hash *hash);

void ipv6_increment_one(
  const void *src_ip, uint8_t bits, struct ip_hash *hash);

static inline int use_small(struct ip_hash *hash)
{
  return hash->initial_tokens <= 65535 && hash->initial_tokens >= 256;
}

static inline int use_tiny(struct ip_hash *hash)
{
  return hash->initial_tokens <= 255;
}

#endif
