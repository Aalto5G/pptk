#ifndef _BYTESWAP_H_
#define _BYTESWAP_H_

static inline uint16_t byteswap16(uint16_t in)
{
  return __builtin_bswap16(in);
}

static inline uint32_t byteswap32(uint32_t in)
{
  return __builtin_bswap32(in);
}

static inline uint64_t byteswap64(uint64_t in)
{
  return __builtin_bswap64(in);
}

#endif
