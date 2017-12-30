#ifndef _ARP_H_
#define _ARP_H_

#include "packet.h"
#include "ports.h"
#include "hashtable.h"
#include "linkedlist.h"
#include "containerof.h"
#include "siphash.h"
#include "hashseed.h"

struct arp_entry {
  struct hash_list_node node;
  uint32_t ip;
  char mac[6];
  int valid;
  struct linked_list_head list;
};

static inline uint32_t ip_hash(uint32_t ip)
{
  return siphash64(hash_seed_get(), ip);
}

static inline uint32_t arp_entry_hash(struct arp_entry *e)
{
  return ip_hash(e->ip);
}

uint32_t arp_entry_hash_fn(struct hash_list_node *node, void *userdata);

struct arp_cache {
  struct hash_table hash;
};

static inline void arp_cache_init(struct arp_cache *cache)
{
  if (hash_table_init(&cache->hash, 1024, arp_entry_hash_fn, NULL) != 0)
  {
    abort();
  }
}

void arp_cache_drain(struct arp_entry *entry, struct port *port);

static inline struct arp_entry *arp_cache_get(
  struct arp_cache *cache, uint32_t ip)
{
  uint32_t hashval = ip_hash(ip);
  struct hash_list_node *node;
  struct arp_entry *entry;
  HASH_TABLE_FOR_EACH_POSSIBLE(&cache->hash, node, hashval)
  {
    entry = CONTAINER_OF(node, struct arp_entry, node);
    if (entry->ip == ip && entry->valid)
    {
      return entry;
    }
  }
  return NULL;
}

void arp_cache_put_packet(
  struct arp_cache *cache, uint32_t ip, struct packet *pkt);

void arp_cache_put(
  struct arp_cache *cache, struct port *port, uint32_t ip, const char *mac);

#endif
