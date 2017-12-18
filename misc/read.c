#include "read.h"
#include <errno.h>
#include <sys/poll.h>
#include <stdio.h>

int accept_interrupt(int fd, struct sockaddr *addr, socklen_t *addrlen, int interruptfd)
{
  char ch;
  struct pollfd pfds[2];
  int ret;
  for (;;)
  {
    pfds[0].fd = fd;
    pfds[1].fd = interruptfd;
    pfds[0].events = POLLIN;
    pfds[1].events = POLLIN;
    poll(pfds, 2, -1);
    if (read(interruptfd, &ch, 1) > 0)
    {
      errno = EINTR;
      return -1;
    }
    ret = accept(fd, addr, addrlen);
    if (ret >= 0)
    {
      return ret;
    }
  }
}

ssize_t readall_interrupt(int fd, void *buf, size_t count, int interruptfd)
{
  size_t bytes_read = 0;
  char *cbuf = buf;
  ssize_t ret;
  struct pollfd pfds[2];
  char ch;
  while (bytes_read < count)
  {
    pfds[0].fd = fd;
    pfds[1].fd = interruptfd;
    pfds[0].events = POLLIN;
    pfds[1].events = POLLIN;
    poll(pfds, 2, -1);
    if (read(interruptfd, &ch, 1) > 0)
    {
      errno = EINTR;
      return -1;
    }
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
