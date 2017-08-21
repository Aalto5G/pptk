#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include "hashlist.h"
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

typedef uint32_t (*hash_fn)(struct hash_list_node *node, void *userdata);

struct hash_table {
  struct hash_list_head *buckets;
  size_t bucketcnt;
  hash_fn fn; 
  void *fn_userdata;
  size_t itemcnt;
};

static inline size_t next_highest_power_of_2(size_t x)
{
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
#if SIZE_MAX > (4U*1024U*1024U*1024U)
  x |= x >> 32;
#endif
  x++;
  return x;
}

static inline int hash_table_init(
  struct hash_table *table, size_t bucketcnt, hash_fn fn, void *fn_userdata)
{
  size_t i;
  table->fn = fn;
  table->fn_userdata = fn_userdata;
  table->itemcnt = 0;
  table->bucketcnt = next_highest_power_of_2(bucketcnt);
  table->buckets = malloc(sizeof(*table->buckets)*table->bucketcnt);
  if (table->buckets == NULL)
  {
    table->fn = NULL;
    table->fn_userdata = NULL;
    table->bucketcnt = 0;
    return -ENOMEM;
  }
  for (i = 0; i < table->bucketcnt; i++)
  {
    hash_list_head_init(&table->buckets[i]);
  }
  return 0;
}

static inline void hash_table_free(struct hash_table *table)
{
  if (table->itemcnt)
  {
    abort();
  }
  free(table->buckets);
  table->buckets = NULL;
  table->bucketcnt = 0;
  table->fn = NULL;
  table->fn_userdata = NULL;
  table->itemcnt = 0;
}

static inline void hash_table_delete(
  struct hash_table *table, struct hash_list_node *node)
{
  hash_list_delete(node);
  if (table->itemcnt == 0)
  {
    abort();
  }
  table->itemcnt--;
}

static inline void hash_table_add_nogrow(
  struct hash_table *table, struct hash_list_node *node, uint32_t hashval)
{
  hash_list_add_head(node, &table->buckets[hashval & (table->bucketcnt - 1)]);
  table->itemcnt++;
}

#define HASH_TABLE_FOR_EACH(table, bucket, x) \
  for ((bucket) = 0, x = NULL; x == NULL && (bucket) < (table)->bucketcnt; (bucket)++) \
    HASH_LIST_FOR_EACH(x, &(table)->buckets[bucket])

#define HASH_TABLE_FOR_EACH_SAFE(table, bucket, x, n) \
  for ((bucket) = 0, x = NULL; x == NULL && (bucket) < (table)->bucketcnt; (bucket)++) \
    HASH_LIST_FOR_EACH_SAFE(x, n, &(table)->buckets[bucket])

#define HASH_TABLE_FOR_EACH_POSSIBLE(table, x, hashval) \
  HASH_LIST_FOR_EACH(x, &((table)->buckets[hashval & ((table)->bucketcnt - 1)]))

#define HASH_TABLE_FOR_EACH_POSSIBLE_SAFE(table, x, n, hashval) \
  HASH_LIST_FOR_EACH_SAFE(x, n, &((table)->buckets[hashval & ((table)->bucketcnt - 1)]))

#endif
