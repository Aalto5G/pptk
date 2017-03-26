#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "databuf.h"

void datainbuf_init(struct datainbuf *inbuf, const void *buf, size_t sz)
{
  inbuf->buf = buf;
  inbuf->size = sz;
  inbuf->location = 0;
  inbuf->errors = 0;
}

void databuf_init(struct databuf *buf)
{
  buf->buf = NULL;
  buf->capacity = 0;
  buf->size = 0;
  buf->errors = 0;
}

int databuf_extend(struct databuf *buf, size_t extend)
{
  size_t new_capacity = buf->size + extend;
  char *new_buf;
  if (new_capacity < buf->capacity*2)
  {
    new_capacity = buf->capacity*2;
  }
  new_buf = realloc(buf->buf, new_capacity);
  if (new_buf == NULL)
  {
    buf->errors = 1;
    return -ENOMEM;
  }
  buf->buf = new_buf;
  buf->capacity = new_capacity;
  return 0;
}
