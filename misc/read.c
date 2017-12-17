#include "read.h"
#include <errno.h>

ssize_t readall(int fd, void *buf, size_t count)
{
  size_t bytes_read = 0;
  char *cbuf = buf;
  ssize_t ret;
  while (bytes_read < count)
  {
    ret = read(fd, cbuf + bytes_read, count - bytes_read);
    if (ret > 0)
    {
      bytes_read += ret;
    }
    if (ret == 0)
    {
      break;
    }
    if (ret < 0 && errno == EINTR)
    {
      continue;
    }
    if (ret < 0)
    {
      return ret;
    }
  }
  return bytes_read;
}
