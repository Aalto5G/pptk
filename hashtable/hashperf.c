#include "siphash.h"
#include "murmur.h"
#include "hashlist.h"
#include "hashtable.h"
#include "containerof.h"
#include <stdio.h>
#include <sys/time.h>

static inline uint64_t gettime64(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec*1000UL*1000UL + tv.tv_usec;
}

struct entry {
  struct hash_list_node node;
  uint32_t ip;
  char mac[6];
};

uint32_t global_hash_seed = 12345;

static inline uint32_t ip_hash(uint32_t ip)
{
  return murmur32(0x12345678, ip);
}

static inline uint32_t entry_hash(struct entry *e)
{
  return ip_hash(e->ip);
}

static inline uint32_t entry_hash_fn(
  struct hash_list_node *node, void *userdata)
{
  return entry_hash(CONTAINER_OF(node, struct entry, node));
}

int main(int argc, char **argv)
{
  struct hash_table table;
  struct entry e1 = {.ip = (10<<24)|(11<<16)|2, .mac = {0x02,0,0,0,0,2}};
  struct entry e2 = {.ip = (10<<24)|(11<<16)|3, .mac = {0x02,0,0,0,0,3}};
  struct entry e3 = {.ip = (10<<24)|(11<<16)|4, .mac = {0x02,0,0,0,0,4}};
  struct entry e4 = {.ip = (10<<24)|(11<<16)|5, .mac = {0x02,0,0,0,0,5}};
  struct entry e5 = {.ip = (10<<24)|(11<<16)|6, .mac = {0x02,0,0,0,0,6}};
  struct hash_list_node *node;
  volatile uint32_t lookup_ip = (10<<24)|(11<<16)|2;
  uint64_t start, end;
  int i;

  hash_table_init(&table, 10, entry_hash_fn, NULL);
  hash_table_add_nogrow(&table, &e1.node, entry_hash(&e1));
  hash_table_add_nogrow(&table, &e2.node, entry_hash(&e2));
  hash_table_add_nogrow(&table, &e3.node, entry_hash(&e3));
  hash_table_add_nogrow(&table, &e4.node, entry_hash(&e4));
  hash_table_add_nogrow(&table, &e5.node, entry_hash(&e5));

  start = gettime64();
  for (i = 0; i < 1000*1000*1000; i++)
  {
    struct entry *e_correct = NULL;
    uint32_t hashval = ip_hash(lookup_ip);
    HASH_TABLE_FOR_EACH_POSSIBLE(&table, node, hashval)
    {
      struct entry *e = CONTAINER_OF(node, struct entry, node);
      if (e->ip == lookup_ip)
      {
        e_correct = e;
        break;
      }
    }
    if (e_correct == NULL)
    {
      abort();
    }
  }
  end = gettime64();
  printf("%g MPPS\n", 1e9/(end-start));

  return 0;
}
