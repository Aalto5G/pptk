#ifndef _CHACHA_H_
#define _CHACHA_H_

#include <stdint.h>

void chacha20_block(
  char key[32], uint32_t counter, char nonce[12], char out[64]);

#endif
