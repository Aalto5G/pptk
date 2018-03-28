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

static inline uint8_t ipv6_traffic_class(const void *pkt)
{
  const char *cpkt = pkt;
  return  (hdr_get8h(&cpkt[0])&0xF) |
         ((hdr_get8h(&cpkt[1])>>4)&0xF);
         
}

static inline void ipv6_set_traffic_class(void *pkt, uint8_t cls)
{
  char *cpkt = pkt;
  uint8_t ch;
  ch = hdr_get8h(&cpkt[0]);
  hdr_set8h(&cpkt[0], (ch&0xF0) | (cls>>4));
  ch = hdr_get8h(&cpkt[1]);
  hdr_set8h(&cpkt[1], (ch&0x0F) | ((cls&0xF) << 4));
}

static inline void ipv6_set_flow_label(void *pkt, uint32_t fl)
{
  char *cpkt = pkt;
  uint8_t ch;
  ch = hdr_get8h(&cpkt[1]);
  hdr_set8h(&cpkt[1], (ch&0xF0) | ((fl>>16)&0xF));
  hdr_set16n(&cpkt[2], fl&0xFFFF);
}

static inline void ipv6_set_payload_length(void *pkt, uint16_t paylen)
{
  char *cpkt = pkt;
  hdr_set16n(&cpkt[4], paylen);
}

static inline void ipv6_set_next_hdr(void *pkt, uint8_t nexthdr)
{
  char *cpkt = pkt;
  hdr_set8h(&cpkt[6], nexthdr);
}

static inline void ipv6_set_hop_limit(void *pkt, uint8_t hl)
{
  char *cpkt = pkt;
  hdr_set8h(&cpkt[7], hl);
}

static inline uint32_t ipv6_flow_label(const void *pkt)
{
  const char *cpkt = pkt;
  return (((hdr_get8h(&cpkt[1]))&0xF)<<16) | hdr_get16n(&cpkt[2]);
}

static inline uint16_t ipv6_payload_length(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get16n(&cpkt[4]);
}

static inline uint8_t ipv6_nexthdr(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get8h(&cpkt[6]);
}

static inline uint8_t ipv6_hop_limit(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get8h(&cpkt[7]);
}

static inline void *ipv6_src(void *pkt)
{
  char *cpkt = pkt;
  return &cpkt[8];
}

static inline void *ipv6_dst(void *pkt)
{
  char *cpkt = pkt;
  return &cpkt[8+16];
}

static inline const void *ipv6_const_src(const void *pkt)
{
  const char *cpkt = pkt;
  return &cpkt[8];
}

static inline const void *ipv6_const_dst(const void *pkt)
{
  const char *cpkt = pkt;
  return &cpkt[8+16];
}

static inline void ip_set_version(void *pkt, uint8_t version)
{
  char *cpkt = pkt;
  unsigned char uch = cpkt[0];
  uch &= ~0xF0;
  uch |= (version & 0xF) << 4;
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

static inline int tcp_ack(const void *pkt)
{
  const char *cpkt = pkt;
  return !!(hdr_get8h(&cpkt[13]) & (1<<4));
}

static inline void tcp_set_ack_on(void *pkt)
{
  char *cpkt = pkt;
  hdr_set8h(&cpkt[13], hdr_get8h(&cpkt[13]) | (1<<4));
}

static inline void tcp_set_ack_off(void *pkt)
{
  char *cpkt = pkt;
  hdr_set8h(&cpkt[13], hdr_get8h(&cpkt[13]) & ~(1<<4));
}

static inline int tcp_rst(const void *pkt)
{
  const char *cpkt = pkt;
  return !!(hdr_get8h(&cpkt[13]) & (1<<2));
}

static inline void tcp_set_rst_on(void *pkt)
{
  char *cpkt = pkt;
  hdr_set8h(&cpkt[13], hdr_get8h(&cpkt[13]) | (1<<2));
}

static inline int tcp_syn(const void *pkt)
{
  const char *cpkt = pkt;
  return !!(hdr_get8h(&cpkt[13]) & (1<<1));
}

static inline void tcp_set_syn_on(void *pkt)
{
  char *cpkt = pkt;
  hdr_set8h(&cpkt[13], hdr_get8h(&cpkt[13]) | (1<<1));
}

static inline int tcp_fin(const void *pkt)
{
  const char *cpkt = pkt;
  return !!(hdr_get8h(&cpkt[13]) & (1<<0));
}

static inline void tcp_set_fin_on(void *pkt)
{
  char *cpkt = pkt;
  hdr_set8h(&cpkt[13], hdr_get8h(&cpkt[13]) | (1<<0));
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

static inline uint32_t tcp_seq_number(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get32n(&cpkt[4]);
}

static inline uint32_t tcp_ack_number(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get32n(&cpkt[8]);
}

static inline void tcp_set_seq_number(void *pkt, uint32_t seq_number)
{
  char *cpkt = pkt;
  hdr_set32n(&cpkt[4], seq_number);
}

static inline void tcp_set_ack_number(void *pkt, uint32_t ack_number)
{
  char *cpkt = pkt;
  hdr_set32n(&cpkt[8], ack_number);
}

static inline uint16_t tcp_window(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get16n(&cpkt[14]);
}

static inline void tcp_set_window(void *pkt, uint16_t window)
{
  char *cpkt = pkt;
  hdr_set16n(&cpkt[14], window);
}

static inline void tcp_set_data_offset(void *pkt, uint8_t data_off)
{
  uint8_t val;
  char *cpkt = pkt;
  if (data_off % 4 != 0)
  {
    abort();
  }
  val = hdr_get8h(&cpkt[12]);
  val &= ~0xF0;
  val |= ((data_off/4)<<4);
  hdr_set8h(&cpkt[12], val);
}

static inline uint8_t tcp_data_offset(const void *pkt)
{
  const char *cpkt = pkt;
  return (hdr_get8h(&cpkt[12])>>4)*4;
}

struct sack_ts_headers {
  uint8_t sackoff;
  uint8_t sacklen;
  uint8_t tsoff;
};

void tcp_find_sack_ts_headers(
  void *pkt, struct sack_ts_headers *hdrs);

void *tcp_find_sack_header(
  void *pkt, size_t *sacklen, int *sixteen_bit_align);

static inline uint32_t tcp_tsval(
  const void *pkt, struct sack_ts_headers *hdrs)
{
  const char *cpkt = pkt;
  if (hdrs->tsoff < 20)
  {
    abort();
  }
  return hdr_get32n(&cpkt[hdrs->tsoff + 2]);
}

static inline uint32_t tcp_tsecho(
  const void *pkt, struct sack_ts_headers *hdrs)
{
  const char *cpkt = pkt;
  if (hdrs->tsoff < 20)
  {
    abort();
  }
  return hdr_get32n(&cpkt[hdrs->tsoff + 6]);
}

struct tcp_information {
  uint8_t options_valid;
  uint8_t wscale;
  uint16_t mss;
  uint8_t sack_permitted;
  uint8_t mssoff; // from beginning of TCP header
  uint8_t ts_present;
  uint32_t ts;
  uint32_t tsecho;
};

void tcp_parse_options(
  void *pkt,
  struct tcp_information *info);

#endif
