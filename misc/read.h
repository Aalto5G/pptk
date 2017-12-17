#ifndef _READ_H_
#define _READ_H_

#include <unistd.h>
#include <stddef.h>

ssize_t readall(int fd, void *buf, size_t count);

#endif
