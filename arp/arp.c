#include <string.h>
#include "arp.h"
#include "iphdr.h"
#include "packet.h"
#include "hashtable.h"
#include "linkedlist.h"
#include "containerof.h"

uint32_t arp_entry_hash_fn(struct hash_list_node *node, void *userdata)
{
  return arp_entry_hash(CONTAINER_OF(node, struct arp_entry, node));
}

void arp_cache_drain(struct arp_entry *entry, struct port *port)
{
  while (!linked_list_is_empty(&entry->list))
  {
    struct linked_list_node *node = entry->list.node.next;
    struct packet *pkt = CONTAINER_OF(node, struct packet, node);
    void *ether = pkt->data;
    memcpy(ether_dst(ether), entry->mac, 6);
    linked_list_delete(node);
    if (port)
    {
      port->portfunc(pkt, port->userdata);
    }
  }
}

#define ARP_CACHE_MAX_PACKETS 40
#define ARP_CACHE_TIMEOUT_SECS 30

void arp_cache_put_packet(
  struct arp_cache *cache, uint32_t ip, struct packet *pkt,
  struct timer_linkheap *timers, uint64_t time64)
{
  uint32_t hashval = ip_hash(ip);
  struct hash_list_node *node;
  struct arp_entry *entry;
  HASH_TABLE_FOR_EACH_POSSIBLE(&cache->hash, node, hashval)
  {
    entry = CONTAINER_OF(node, struct arp_entry, node);
    if (entry->ip == ip)
    {
      while (linked_list_size(&entry->list) >= ARP_CACHE_MAX_PACKETS)
      {
        struct packet *pkt2 =
          CONTAINER_OF(entry->list.node.next, struct packet, node);
        linked_list_delete(&pkt2->node);
        allocif_free(cache->intf, pkt2);
      }
      linked_list_add_tail(&pkt->node, &entry->list);
      entry->timer.time64 = time64 + ARP_CACHE_TIMEOUT_SECS*1000ULL*1000ULL;
      timer_linkheap_modify(timers, &entry->timer);
      return;
    }
  }
  entry = malloc(sizeof(*entry));
  memset(entry, 0, sizeof(*entry));
  entry->ip = ip;
  memset(entry->mac, 0, sizeof(entry->mac));
  linked_list_head_init(&entry->list);
  linked_list_add_tail(&pkt->node, &entry->list);
  hash_table_add_nogrow(&cache->hash, &entry->node, hashval);
  entry->timer.time64 = time64 + ARP_CACHE_TIMEOUT_SECS*1000ULL*1000ULL;
  entry->timer.fn = arp_entry_expiry_fn;
  entry->timer.userdata = timers;
  timer_linkheap_add(timers, &entry->timer);
}

void arp_cache_put(
  struct arp_cache *cache, struct port *port, uint32_t ip, const char *mac,
  struct timer_linkheap *timers, uint64_t time64)
{
  uint32_t hashval = ip_hash(ip);
  struct hash_list_node *node;
  struct arp_entry *entry;
  HASH_TABLE_FOR_EACH_POSSIBLE(&cache->hash, node, hashval)
  {
    entry = CONTAINER_OF(node, struct arp_entry, node);
    if (entry->ip == ip)
    {
      memcpy(entry->mac, mac, 6);
      entry->valid = 1;
      arp_cache_drain(entry, port);
      entry->timer.time64 = time64 + ARP_CACHE_TIMEOUT_SECS*1000ULL*1000ULL;
      timer_linkheap_modify(timers, &entry->timer);
      return;
    }
  }
  entry = malloc(sizeof(*entry));
  memset(entry, 0, sizeof(*entry));
  entry->ip = ip;
  entry->valid = 1;
  memcpy(entry->mac, mac, 6);
  linked_list_head_init(&entry->list);
  hash_table_add_nogrow(&cache->hash, &entry->node, hashval);
  entry->timer.time64 = time64 + ARP_CACHE_TIMEOUT_SECS*1000ULL*1000ULL;
  entry->timer.fn = arp_entry_expiry_fn;
  entry->timer.userdata = timers;
  timer_linkheap_add(timers, &entry->timer);
}

void arp_entry_expiry_fn(struct timer_link *timer, struct timer_linkheap *heap, void *userdata, void *threaddata)
{
  struct arp_entry *entry = CONTAINER_OF(timer, struct arp_entry, timer);
  struct arp_cache *cache = userdata;
  while (!linked_list_is_empty(&entry->list))
  {
    struct packet *pkt2 =
      CONTAINER_OF(entry->list.node.next, struct packet, node);
    linked_list_delete(&pkt2->node);
    allocif_free(cache->intf, pkt2);
  }
  hash_table_delete_already_bucket_locked(&cache->hash, &entry->node);
  free(entry);
}
