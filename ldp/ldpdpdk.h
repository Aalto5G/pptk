#ifndef _LDP_DPDK_H_
#define _LDP_DPDK_H_

#include "ldp.h"

struct ldp_interface *
ldp_interface_open_dpdk(const char *name, int numinq, int numoutq);

#endif
