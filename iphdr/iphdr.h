#ifndef _IPHDR_H_
#define _IPHDR_H_

#include "hdr.h"

static inline void *etherDst(void *pkt)
{
  char *cpkt = pkt;
  return &cpkt[0];
}

static inline void *etherSrc(void *pkt)
{
  char *cpkt = pkt;
  return &cpkt[6];
}

static inline const void *etherConstDst(const void *pkt)
{
  const char *cpkt = pkt;
  return &cpkt[0];
}

static inline const void *etherConstSrc(const void *pkt)
{
  const char *cpkt = pkt;
  return &cpkt[6];
}

static inline uint16_t etherType(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get16n(&cpkt[12]);
}

static inline void etherSetType(void *pkt, uint16_t type)
{
  char *cpkt = pkt;
  hdr_set16n(&cpkt[12], type);
}

#define ETHER_TYPE_IP ((uint16_t)0x0800)
#define ETHER_TYPE_ARP ((uint16_t)0x0806)
#define ETHER_TYPE_IPV6 ((uint16_t)0x86DD)

static inline uint8_t ipVersion(const void *pkt)
{
  const char *cpkt = pkt;
  return (hdr_get8h(&cpkt[0])>>4)&0xF;
}

static inline uint8_t ipHdrLen(const void *pkt)
{
  const char *cpkt = pkt;
  return (hdr_get8h(&cpkt[0]))&0xF;
}

static inline uint16_t ipTotalLen(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get16n(&cpkt[2]);
}

static inline int ipMoreFrags(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get8h(&cpkt[6]) & (1<<5);
}

static inline int ipDontFrag(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get8h(&cpkt[6]) & (1<<6);
}

static inline int ipRsvdBit(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get8h(&cpkt[6]) & (1<<7);
}

static inline uint16_t ipID(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get16n(&cpkt[4]);
}

static inline uint16_t ipFragOff(const void *pkt)
{
  const char *cpkt = pkt;
  return (hdr_get16n(&cpkt[6])&0x1FFF)*8;
}

static inline uint8_t ipTTL(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get8h(&cpkt[8]);
}

static inline uint8_t ipProto(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get8h(&cpkt[9]);
}

static inline uint16_t ipHdrCksum(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get16n(&cpkt[10]);
}

static inline uint32_t ipSrc(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get32n(&cpkt[12]);
}

static inline uint32_t ipDst(const void *pkt)
{
  const char *cpkt = pkt;
  return hdr_get32n(&cpkt[16]);
}

#endif
