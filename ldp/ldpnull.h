#ifndef _LDP_NULL_H_
#define _LDP_NULL_H_

#include "ldp.h"

struct ldp_interface *
ldp_interface_open_null(const char *name, int numinq, int numoutq);

#endif
