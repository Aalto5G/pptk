/*
 * Authors:
 * - Juha-Matti Tilli <juha-matti.tilli@iki.fi>
 *
 * This file is covered by GNU GENERAL PUBLIC LICENSE Version 2.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "llalloc.h"
#include "iphdr.h"
#include "packet.h"
#include "ipcksum.h"
#include "ipfrag.h"
#include "containerof.h"
#include "linux.h"

int main(int argc, char **argv)
{
  struct ll_alloc_st st;
  struct allocif intf = {.ops = &ll_allocif_ops_st, .userdata = &st};
  struct fragment fragment[2];
  char pkt[2102] = {0};
  size_t sz = sizeof(pkt);
  char *ether = pkt;
  char *ip;
  char *tcp;
  char edst[6] = {0x02,0x00,0x00,0x00,0x00,0x01};
  char esrc[6] = {0x02,0x00,0x00,0x00,0x00,0x02};
  struct packet *reassembled;
  struct ipq ipq;
  int i, j;

  memcpy(ether_dst(ether), edst, 6);
  memcpy(ether_src(ether), esrc, 6);
  ether_set_type(ether, ETHER_TYPE_IP);
  ip = ether_payload(ether);
  ip_set_version(ip, 4);
  ip_set_hdr_len(ip, 20);
  ip_set_total_len(ip, sizeof(pkt) - 14);
  ip_set_id(ip, 0x1234);
  ip_set_ttl(ip, 64);
  ip_set_proto(ip, 6);
  ip_set_src(ip, (10<<24) | 1);
  ip_set_dst(ip, (10<<24) | 2);
  ip_set_hdr_cksum_calc(ip, 20);
  tcp = ip_payload(ip);
  tcp_set_src_port(tcp, 12345);
  tcp_set_dst_port(tcp, 54321);
  tcp_set_ack_on(tcp);
  tcp_set_seq_number(tcp, 0x12345678);
  tcp_set_ack_number(tcp, 0x87654321);
  tcp_set_window(tcp, 32768);
  tcp_set_data_offset(tcp, 20);
  memset(((char*)tcp) + 20, 'X', sizeof(pkt)-14-20-20);
  tcp_set_cksum_calc(ip, 20, tcp, sizeof(pkt)-14-20);
  ll_alloc_st_init(&st, 1000, 1536);
  fragment[0].datastart = 0;
  fragment[0].datalen = 1514 - 14 - 20;
  fragment[0].pkt = NULL;
  fragment[1].datastart = 1514 - 14 - 20;
  fragment[1].datalen = sz - 14 - 20 - (1514 - 14 - 20);
  fragment[1].pkt = NULL;
  if (fragment4(&intf, pkt, sz, fragment, 2) != 0)
  {
    abort();
  }

  ipq_init(&ipq);
  if (ip_frag_queue(&intf, &ipq, fragment[0].pkt) != -EINPROGRESS)
  {
    abort();
  }
  if (ip_frag_queue(&intf, &ipq, fragment[1].pkt) != 0)
  {
    abort();
  }
  reassembled = ip_frag_reassemble(&intf, &ipq);
  if (reassembled->sz != sz)
  {
    abort();
  }
  if (memcmp(packet_data(reassembled), pkt, sz) != 0)
  {
    abort();
  }
  allocif_free(&intf, reassembled);

  ipq_init(&ipq);
  if (ip_frag_queue(&intf, &ipq, fragment[1].pkt) != -EINPROGRESS)
  {
    abort();
  }
  if (ip_frag_queue(&intf, &ipq, fragment[0].pkt) != 0)
  {
    abort();
  }
  reassembled = ip_frag_reassemble(&intf, &ipq);
  if (reassembled->sz != sz)
  {
    abort();
  }
  if (memcmp(packet_data(reassembled), pkt, sz) != 0)
  {
    abort();
  }
  allocif_free(&intf, reassembled);
  
  allocif_free(&intf, fragment[0].pkt);
  allocif_free(&intf, fragment[1].pkt);

  for (j = 0; j < 10*1000; j++)
  {
    printf("seeding random %d\n", j);
    srand(j);
    ipq_init(&ipq);
    i = 0;
    for (;;)
    {
      fragment[0].datastart = ((rand() % (sz - 14 - 20)) >> 3) << 3;
      if (rand() % 2)
      {
        fragment[0].datalen = 0;
      }
      else
      {
        fragment[0].datalen = 1 + (rand() % (sz - 14 - 20 - fragment[0].datastart));
      }
      fragment[0].pkt = NULL;
      if (fragment4(&intf, pkt, sz, fragment, 1) != 0)
      {
        abort();
      }
      if (ip_frag_queue(&intf, &ipq, fragment[0].pkt) == 0)
      {
        break;
      }
      i++;
    }
    printf("%d packets\n", i);
    reassembled = ip_frag_reassemble(&intf, &ipq);
    if (reassembled->sz != sz)
    {
      printf("size mismatch %zu %zu\n", reassembled->sz, sz);
      abort();
    }
    if (memcmp(packet_data(reassembled), pkt, sz) != 0)
    {
      printf("packet data mismatch\n");
      abort();
    }
    allocif_free(&intf, reassembled);
    ipq_free(&intf, &ipq);
  }

  ll_alloc_st_free(&st);
  
  return 0;
}
