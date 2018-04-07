#ifndef _LDP_DPDK_H_
#define _LDP_DPDK_H_

#include "ldp.h"

struct ldp_interface *
ldp_interface_open_dpdk(const char *name, int numinq, int numoutq);

int ldp_dpdk_mac_addr(int portid, void *mac);

int ldp_dpdk_promisc_mode_set(int portid, int on);

#endif
