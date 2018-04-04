#ifndef _READ_H_
#define _READ_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stddef.h>

int accept_interrupt(int fd, struct sockaddr *addr, socklen_t *addrlen, int interruptfd);

int accept_interrupt_dual(int fd, int fd6, struct sockaddr *addr, socklen_t *addrlen, int interruptfd, int *descriptor);

ssize_t readall(int fd, void *buf, size_t count);

ssize_t readall_interrupt(int fd, void *buf, size_t count, int interruptfd);

#endif
