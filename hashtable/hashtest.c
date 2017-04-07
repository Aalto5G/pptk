#include "siphash.h"
#include "hashlist.h"
#include "hashtable.h"
#include "containerof.h"
#include <stdio.h>

struct entry {
  struct hash_list_node node;
  uint32_t x;
  uint32_t y;
};

uint32_t global_hash_seed = 12345;

static inline uint32_t entry_hash(struct entry *e)
{
  char key[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
  struct siphash_ctx ctx;
  siphash_init(&ctx, key);
  siphash_feed_u64(&ctx, (((uint64_t)e->x)<<32) | e->y);
  return siphash_get(&ctx);
}

static inline uint32_t entry_hash_fn(
  struct hash_list_node *node, void *userdata)
{
  return entry_hash(CONTAINER_OF(node, struct entry, node));
}

int main(int argc, char **argv)
{
  struct hash_table table;
  struct entry e1 = {.x = 1, .y = 2};
  struct entry e2 = {.x = 3, .y = 4};
  struct entry e3 = {.x = 5, .y = 6};
  struct entry e4 = {.x = 7, .y = 8};
  struct hash_list_node *node;
  uint32_t hashval = entry_hash(&e4);

  hash_table_init(&table, 10, entry_hash_fn, NULL);
  hash_table_add_nogrow(&table, &e1.node, entry_hash(&e1));
  hash_table_add_nogrow(&table, &e2.node, entry_hash(&e2));
  hash_table_add_nogrow(&table, &e3.node, entry_hash(&e3));
  hash_table_add_nogrow(&table, &e4.node, entry_hash(&e4));
  HASH_TABLE_FOR_EACH_POSSIBLE(&table, node, hashval)
  {
    struct entry *e = CONTAINER_OF(node, struct entry, node);
    printf("candidate\n");
    if (e->x == e4.x && e->y == e4.y)
    {
      printf("ok\n");
    }
  }
  hash_table_delete(&table, &e1.node);
  hash_table_delete(&table, &e2.node);
  hash_table_delete(&table, &e3.node);
  hash_table_delete(&table, &e4.node);
  hash_table_free(&table);
  printf("====\n");

  hash_table_init(&table, 1, entry_hash_fn, NULL);
  hash_table_add_nogrow(&table, &e1.node, entry_hash(&e1));
  hash_table_add_nogrow(&table, &e2.node, entry_hash(&e2));
  hash_table_add_nogrow(&table, &e3.node, entry_hash(&e3));
  hash_table_add_nogrow(&table, &e4.node, entry_hash(&e4));
  HASH_TABLE_FOR_EACH_POSSIBLE(&table, node, hashval)
  {
    struct entry *e = CONTAINER_OF(node, struct entry, node);
    printf("candidate\n");
    if (e->x == e4.x && e->y == e4.y)
    {
      printf("ok\n");
    }
  }
  hash_table_delete(&table, &e1.node);
  hash_table_delete(&table, &e2.node);
  hash_table_delete(&table, &e3.node);
  hash_table_delete(&table, &e4.node);
  hash_table_free(&table);
  printf("====\n");

  hash_table_init(&table, 1, entry_hash_fn, NULL);
  hash_table_add_nogrow(&table, &e1.node, entry_hash(&e1));
  hash_table_add_nogrow(&table, &e2.node, entry_hash(&e2));
  hash_table_add_nogrow(&table, &e3.node, entry_hash(&e3));
  hash_table_add_nogrow(&table, &e4.node, entry_hash(&e4));
  HASH_TABLE_FOR_EACH_POSSIBLE(&table, node, hashval)
  {
    struct entry *e = CONTAINER_OF(node, struct entry, node);
    printf("candidate\n");
    if (e->x == e4.x && e->y == e4.y)
    {
      printf("ok\n");
    }
  }
  hash_table_delete(&table, &e1.node);
  hash_table_delete(&table, &e2.node);
  hash_table_delete(&table, &e3.node);
  hash_table_delete(&table, &e4.node);
  hash_table_free(&table);

  return 0;
}
