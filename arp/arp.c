#include <string.h>
#include "arp.h"
#include "iphdr.h"
#include "packet.h"
#include "hashtable.h"
#include "linkedlist.h"
#include "containerof.h"

const char arphashkey[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};

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
    void *ether = packet_data(pkt);
    memcpy(ether_dst(ether), entry->mac, 6);
    linked_list_delete(node);
    if (port)
    {
      port->portfunc(pkt, port->userdata);
    }
  }
}

#define ARP_CACHE_MAX_PACKETS 40

void arp_cache_put_packet(
  struct arp_cache *cache, uint32_t ip, struct packet *pkt)
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
        free(pkt2); // FIXME replace with better allocator
      }
      linked_list_add_tail(&pkt->node, &entry->list);
      return;
    }
  }
  entry = malloc(sizeof(*entry));
  entry->ip = ip;
  linked_list_head_init(&entry->list);
  linked_list_add_tail(&pkt->node, &entry->list);
  hash_table_add_nogrow(&cache->hash, &entry->node, hashval);
}

void arp_cache_put(
  struct arp_cache *cache, struct port *port, uint32_t ip, const char *mac)
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
      return;
    }
  }
  entry = malloc(sizeof(*entry));
  entry->ip = ip;
  entry->valid = 1;
  memcpy(entry->mac, mac, 6);
  linked_list_head_init(&entry->list);
  hash_table_add_nogrow(&cache->hash, &entry->node, hashval);
}
