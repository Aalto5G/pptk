#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "databuf.h"

int main(int argc, char **argv)
{
  struct databuf buf = DATABUF_INITER;
  struct datainbuf inbuf;
  char bytes[3];
  uint32_t u32;
  uint64_t u64;
  if (databuf_add_u32(&buf, 1))
  {
    abort();
  }
  if (databuf_add_u64(&buf, 2))
  {
    abort();
  }
  if (databuf_add_bytes(&buf, "foo", 3))
  {
    abort();
  }
  if (databuf_add_u64(&buf, 3))
  {
    abort();
  }
  if (databuf_add_u32(&buf, 4))
  {
    abort();
  }
  if (buf.errors)
  {
    abort();
  }
  datainbuf_init(&inbuf, buf.buf, buf.size);
  if (datainbuf_get_u32(&inbuf, &u32))
  {
    abort();
  }
  if (u32 != 1)
  {
    abort();
  }
  if (datainbuf_get_u64(&inbuf, &u64))
  {
    abort();
  }
  if (u64 != 2)
  {
    abort();
  }
  datainbuf_get_bytes(&inbuf, bytes, sizeof(bytes));
  if (memcmp(bytes, "foo", 3) != 0)
  {
    abort();
  }
  if (datainbuf_get_u64(&inbuf, &u64))
  {
    abort();
  }
  if (u64 != 3)
  {
    abort();
  }
  if (datainbuf_get_u32(&inbuf, &u32))
  {
    abort();
  }
  if (u32 != 4)
  {
    abort();
  }
  if (inbuf.errors)
  {
    abort();
  }
  if (!datainbuf_eof(&inbuf))
  {
    abort();
  }
}
