#ifndef _CONTAINEROF_H_
#define _CONTAINEROF_H_

#define CONTAINER_OF(ptr, type, member) \
  ((type*)(((char*)ptr) - (((char*)&(((type*)0)->member)) - ((char*)0))))

#endif
