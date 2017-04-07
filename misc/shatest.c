/*
 * WARNING WARNING WARNING! This code is copypasted from Linux kernel, and
 * therefore, it's infected by the General Public Virus version 2. This
 * is here just to demonstrate how slow SHA-1 hashing is.
 */

#include <stdint.h>
#include <string.h>

#define SHA_DIGEST_WORDS 5
#define SHA_WORKSPACE_WORDS 16

static inline uint32_t __get_unaligned_be32(const uint8_t *p)
{
        return p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
}

static inline uint32_t get_unaligned_be32(const void *p)
{
        return __get_unaligned_be32((const uint8_t *)p);
}

static inline uint32_t rol32(uint32_t word, unsigned int shift)
{
        return (word << shift) | (word >> ((-shift) & 31));
}

/**
 * ror32 - rotate a 32-bit value right
 * @word: value to rotate
 * @shift: bits to roll
 */
static inline uint32_t ror32(uint32_t word, unsigned int shift)
{
        return (word >> shift) | (word << (32 - shift));
}

#define setW(x, val) (*(volatile uint32_t *)&W(x) = (val))

/* This "rolls" over the 512-bit array */
#define W(x) (array[(x)&15])

#define SHA_SRC(t) get_unaligned_be32((uint32_t *)data + t)
#define SHA_MIX(t) rol32(W(t+13) ^ W(t+8) ^ W(t+2) ^ W(t), 1)

#define SHA_ROUND(t, input, fn, constant, A, B, C, D, E) do { \
        uint32_t TEMP = input(t); setW(t, TEMP); \
        E += TEMP + rol32(A,5) + (fn) + (constant); \
        B = ror32(B, 2); } while (0)

#define T_0_15(t, A, B, C, D, E)  SHA_ROUND(t, SHA_SRC, (((C^D)&B)^D) , 0x5a827999, A, B, C, D, E )
#define T_16_19(t, A, B, C, D, E) SHA_ROUND(t, SHA_MIX, (((C^D)&B)^D) , 0x5a827999, A, B, C, D, E )
#define T_20_39(t, A, B, C, D, E) SHA_ROUND(t, SHA_MIX, (B^C^D) , 0x6ed9eba1, A, B, C, D, E )
#define T_40_59(t, A, B, C, D, E) SHA_ROUND(t, SHA_MIX, ((B&C)+(D&(B^C))) , 0x8f1bbcdc, A, B, C, D, E )
#define T_60_79(t, A, B, C, D, E) SHA_ROUND(t, SHA_MIX, (B^C^D) ,  0xca62c1d6, A, B, C, D, E )


uint32_t ipv4_cookie_scratch[16 + 5 + SHA_WORKSPACE_WORDS];

static void sha_transform(uint32_t *digest, const char *data, uint32_t *array)
{
        uint32_t A, B, C, D, E;

        A = digest[0];
        B = digest[1];
        C = digest[2];
        D = digest[3];
        E = digest[4];

        /* Round 1 - iterations 0-16 take their input from 'data' */
        T_0_15( 0, A, B, C, D, E);
        T_0_15( 1, E, A, B, C, D);
        T_0_15( 2, D, E, A, B, C);
        T_0_15( 3, C, D, E, A, B);
        T_0_15( 4, B, C, D, E, A);
        T_0_15( 5, A, B, C, D, E);
        T_0_15( 6, E, A, B, C, D);
        T_0_15( 7, D, E, A, B, C);
        T_0_15( 8, C, D, E, A, B);
        T_0_15( 9, B, C, D, E, A);
        T_0_15(10, A, B, C, D, E);
        T_0_15(11, E, A, B, C, D);
        T_0_15(12, D, E, A, B, C);
        T_0_15(13, C, D, E, A, B);
        T_0_15(14, B, C, D, E, A);
        T_0_15(15, A, B, C, D, E);

        /* Round 1 - tail. Input from 512-bit mixing array */
        T_16_19(16, E, A, B, C, D);
        T_16_19(17, D, E, A, B, C);
        T_16_19(18, C, D, E, A, B);
        T_16_19(19, B, C, D, E, A);

        /* Round 2 */
        T_20_39(20, A, B, C, D, E);
        T_20_39(21, E, A, B, C, D);
        T_20_39(22, D, E, A, B, C);
        T_20_39(23, C, D, E, A, B);
        T_20_39(24, B, C, D, E, A);
        T_20_39(25, A, B, C, D, E);
        T_20_39(26, E, A, B, C, D);
        T_20_39(27, D, E, A, B, C);
        T_20_39(28, C, D, E, A, B);
        T_20_39(29, B, C, D, E, A);
        T_20_39(30, A, B, C, D, E);
        T_20_39(31, E, A, B, C, D);
        T_20_39(32, D, E, A, B, C);
        T_20_39(33, C, D, E, A, B);
        T_20_39(34, B, C, D, E, A);
        T_20_39(35, A, B, C, D, E);
        T_20_39(36, E, A, B, C, D);
        T_20_39(37, D, E, A, B, C);
        T_20_39(38, C, D, E, A, B);
        T_20_39(39, B, C, D, E, A);

        /* Round 3 */
        T_40_59(40, A, B, C, D, E);
        T_40_59(41, E, A, B, C, D);
        T_40_59(42, D, E, A, B, C);
        T_40_59(43, C, D, E, A, B);
        T_40_59(44, B, C, D, E, A);
        T_40_59(45, A, B, C, D, E);
        T_40_59(46, E, A, B, C, D);
        T_40_59(47, D, E, A, B, C);
        T_40_59(48, C, D, E, A, B);
        T_40_59(49, B, C, D, E, A);
        T_40_59(50, A, B, C, D, E);
        T_40_59(51, E, A, B, C, D);
        T_40_59(52, D, E, A, B, C);
        T_40_59(53, C, D, E, A, B);
        T_40_59(54, B, C, D, E, A);
        T_40_59(55, A, B, C, D, E);
        T_40_59(56, E, A, B, C, D);
        T_40_59(57, D, E, A, B, C);
        T_40_59(58, C, D, E, A, B);
        T_40_59(59, B, C, D, E, A);

        /* Round 4 */
        T_60_79(60, A, B, C, D, E);
        T_60_79(61, E, A, B, C, D);
        T_60_79(62, D, E, A, B, C);
        T_60_79(63, C, D, E, A, B);
        T_60_79(64, B, C, D, E, A);
        T_60_79(65, A, B, C, D, E);
        T_60_79(66, E, A, B, C, D);
        T_60_79(67, D, E, A, B, C);
        T_60_79(68, C, D, E, A, B);
        T_60_79(69, B, C, D, E, A);
        T_60_79(70, A, B, C, D, E);
        T_60_79(71, E, A, B, C, D);
        T_60_79(72, D, E, A, B, C);
        T_60_79(73, C, D, E, A, B);
        T_60_79(74, B, C, D, E, A);
        T_60_79(75, A, B, C, D, E);
        T_60_79(76, E, A, B, C, D);
        T_60_79(77, D, E, A, B, C);
        T_60_79(78, C, D, E, A, B);
        T_60_79(79, B, C, D, E, A);

        digest[0] += A;
        digest[1] += B;
        digest[2] += C;
        digest[3] += D;
        digest[4] += E;
}

uint32_t syncookie_secret[2][16-4+SHA_DIGEST_WORDS];

int main(int argc, char **argv)
{
  uint32_t saddr = 0x12345678U;
  uint32_t daddr = 0x87654321U;
  uint16_t sport = 12345;
  uint16_t dport = 54321;
  uint32_t *tmp = ipv4_cookie_scratch;
  int c = 0;
  int i;
  uint32_t count = 0;
  for (i = 0; i < 100*1000*1000; i++)
  {
    memcpy(tmp + 4, syncookie_secret[c], sizeof(syncookie_secret[c]));
    tmp[0] = (uint32_t)saddr;
    tmp[1] = (uint32_t)daddr;
    tmp[2] = ((uint32_t)sport << 16) + (uint32_t)dport;
    tmp[3] = count;
    sha_transform(tmp + 16, (char*)(uint32_t *)tmp, tmp + 16 + 5);
  }
}
