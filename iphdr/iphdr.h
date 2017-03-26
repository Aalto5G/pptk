#ifndef _IPHDR_H_
#define _IPHDR_H_

#include "hdr.h"
#include <stdlib.h>

static inline void *ether_dst(void *pkt)
{
  char *cpkt = pkt;
  return &cpkt[0];
}

static inline void *ether_src(void *pkt)
{
  char *cpkt = pkt;
  return &cpkt[6];
}

static inline const void *ether_const_dst(const void *pkt)
{
  const char *cpkt = pkt;
  return &cpkt[0];
}

static inline const void *ether_const_src(const void *pkt)
{
  const char *cpkt = pkt;
  return &cpkt[6];
}

static inline uint16_t ether_type(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get16n(&cpkt[12]);
}

static inline void ether_set_type(void *pkt, uint16_t type)
{
  char *cpkt = pkt;
  hdr_set16n(&cpkt[12], type);
}

static inline void *ether_payload(void *pkt)
{
  char *cpkt = pkt;
  return &cpkt[14];
}

static inline const void *ether_const_payload(const void *pkt)
{
  const char *cpkt = pkt;
  return &cpkt[14];
}

#define ETHER_HDR_LEN 14

#define ETHER_TYPE_IP ((uint16_t)0x0800)
#define ETHER_TYPE_ARP ((uint16_t)0x0806)
#define ETHER_TYPE_IPV6 ((uint16_t)0x86DD)

#define IP_HDR_MINLEN 20

static inline uint8_t ip_version(const void *pkt)
{
  const char *cpkt = pkt;
  return (hdr_get8h(&cpkt[0])>>4)&0xF;
}

static inline void ip_set_version(void *pkt, uint8_t version)
{
  char *cpkt = pkt;
  unsigned char uch = cpkt[0];
  uch &= ~0xF;
  uch |= version & 0xF;
  cpkt[0] = uch;
}

static inline uint8_t ip_hdr_len(const void *pkt)
{
  const char *cpkt = pkt;
  return ((hdr_get8h(&cpkt[0]))&0xF)*4;
}

static inline void ip_set_hdr_len(void *pkt, uint8_t hdr_len)
{
  char *cpkt = pkt;
  unsigned char uch = cpkt[0];
  if (hdr_len % 4 != 0)
  {
    abort();
  }
  hdr_len /= 4;
  uch &= ~0xF;
  uch |= hdr_len & 0xF;
  cpkt[0] = uch;
}

static inline uint16_t ip_total_len(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get16n(&cpkt[2]);
}

static inline void ip_set_total_len(void *pkt, uint16_t total_len)
{
  char *cpkt = pkt;
  hdr_set16n(&cpkt[2], total_len);
}

static inline int ip_more_frags(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get8h(&cpkt[6]) & (1<<5);
}

static inline void ip_set_more_frags(void *pkt, int bit)
{
  char *cpkt = pkt;
  uint8_t u8;
  bit = !!bit;
  u8 = hdr_get8h(&cpkt[6]);
  u8 &= ~(1<<5);
  u8 |= bit<<5;
  hdr_set8h(&cpkt[6], u8);
}

static inline int ip_dont_frag(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get8h(&cpkt[6]) & (1<<6);
}

static inline void ip_set_dont_frag(void *pkt, int bit)
{
  char *cpkt = pkt;
  uint8_t u8;
  bit = !!bit;
  u8 = hdr_get8h(&cpkt[6]);
  u8 &= ~(1<<6);
  u8 |= bit<<6;
  hdr_set8h(&cpkt[6], u8);
}

static inline int ip_rsvd_bit(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get8h(&cpkt[6]) & (1<<7);
}

static inline void ip_set_rsvd_bit(void *pkt, int bit)
{
  char *cpkt = pkt;
  uint8_t u8;
  bit = !!bit;
  u8 = hdr_get8h(&cpkt[6]);
  u8 &= ~(1<<7);
  u8 |= bit<<7;
  hdr_set8h(&cpkt[6], u8);
}

static inline uint16_t ip_id(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get16n(&cpkt[4]);
}

static inline void ip_set_id(void *pkt, uint16_t id)
{
  char *cpkt = pkt;
  hdr_set16n(&cpkt[4], id);
}

static inline uint16_t ip_frag_off(const void *pkt)
{
  const char *cpkt = pkt;
  return (hdr_get16n(&cpkt[6])&0x1FFF)*8;
}

static inline void ip_set_frag_off(void *pkt, uint16_t frag_off)
{
  char *cpkt = pkt;
  unsigned char c6;
  if (frag_off % 8 != 0)
  {
    abort();
  }
  frag_off /= 8;
  c6 = (unsigned char)cpkt[6];
  c6 &= ~0x1F;
  c6 |= (frag_off>>8) & 0x1F;
  cpkt[6] = (char)c6;
  cpkt[7] = (char)(unsigned char)(frag_off&0xFF);
}

static inline uint8_t ip_ttl(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get8h(&cpkt[8]);
}

static inline void ip_set_ttl(void *pkt, uint8_t ttl)
{
  char *cpkt = pkt;
  hdr_set8h(&cpkt[8], ttl);
}

static inline int ip_decr_ttl(void *pkt)
{
  uint8_t ttl;
  ttl = ip_ttl(pkt);
  if (ttl == 0)
  {
    abort();
  }
  ttl--;
  ip_set_ttl(pkt, ttl);
  return ttl > 0;
}

static inline uint8_t ip_proto(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get8h(&cpkt[9]);
}

static inline void ip_set_proto(void *pkt, uint8_t proto)
{
  char *cpkt = pkt;
  hdr_set8h(&cpkt[9], proto);
}

static inline uint16_t ip_hdr_cksum(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get16n(&cpkt[10]);
}

static inline void ip_set_hdr_cksum(void *pkt, uint16_t hdr_cksum)
{
  char *cpkt = pkt;
  hdr_set16n(&cpkt[10], hdr_cksum);
}

static inline uint32_t ip_src(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get32n(&cpkt[12]);
}

static inline uint32_t ip_dst(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get32n(&cpkt[16]);
}

static inline void ip_set_src(void *pkt, uint32_t src)
{
  char *cpkt = pkt;
  return hdr_set32n(&cpkt[12], src);
}

static inline void ip_set_dst(void *pkt, uint32_t dst)
{
  char *cpkt = pkt;
  return hdr_set32n(&cpkt[16], dst);
}

static inline void *ip_payload(void *pkt)
{
  return ((char*)pkt) + ip_hdr_len(pkt);
}

static inline const void *ip_const_payload(const void *pkt)
{
  return ((const char*)pkt) + ip_hdr_len(pkt);
}

static inline uint16_t tcp_src_port(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get16n(&cpkt[0]);
}

static inline uint16_t tcp_dst_port(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get16n(&cpkt[2]);
}

static inline void tcp_set_src_port(void *pkt, uint16_t src_port)
{
  char *cpkt = pkt;
  hdr_set16n(&cpkt[0], src_port);
}

static inline void tcp_set_dst_port(void *pkt, uint16_t dst_port)
{
  char *cpkt = pkt;
  hdr_set16n(&cpkt[2], dst_port);
}

static inline uint32_t tcp_seq_num(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get32n(&cpkt[4]);
}

static inline uint32_t tcp_ack_num(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get32n(&cpkt[8]);
}

static inline int tcp_ack(const void *pkt)
{
  const char *cpkt = pkt;
  return !!(hdr_get8h(&cpkt[13]) & (1<<4));
}

static inline int tcp_rst(const void *pkt)
{
  const char *cpkt = pkt;
  return !!(hdr_get8h(&cpkt[13]) & (1<<2));
}

static inline int tcp_syn(const void *pkt)
{
  const char *cpkt = pkt;
  return !!(hdr_get8h(&cpkt[13]) & (1<<1));
}

static inline int tcp_fin(const void *pkt)
{
  const char *cpkt = pkt;
  return !!(hdr_get8h(&cpkt[13]) & (1<<0));
}

static inline uint16_t tcp_cksum(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get16n(&cpkt[16]);
}

static inline void tcp_set_cksum(void *pkt, uint16_t checksum)
{
  char *cpkt = pkt;
  hdr_set16n(&cpkt[16], checksum);
}

static inline uint16_t udp_src_port(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get16n(&cpkt[0]);
}

static inline uint16_t udp_dst_port(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get16n(&cpkt[2]);
}

static inline void udp_set_src_port(void *pkt, uint16_t src_port)
{
  char *cpkt = pkt;
  hdr_set16n(&cpkt[0], src_port);
}

static inline void udp_set_dst_port(void *pkt, uint16_t dst_port)
{
  char *cpkt = pkt;
  hdr_set16n(&cpkt[2], dst_port);
}

static inline uint16_t udp_total_len(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get16n(&cpkt[4]);
}

static inline uint16_t udp_cksum(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get16n(&cpkt[6]);
}

static inline void udp_set_cksum(void *pkt, uint16_t cksum)
{
  char *cpkt = pkt;
  hdr_set16n(&cpkt[6], cksum);
}

#endif
