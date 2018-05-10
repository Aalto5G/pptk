#ifndef _LDP_DPDK_H_
#define _LDP_DPDK_H_

#include "ldp.h"

struct ldp_interface *
ldp_interface_open_dpdk(const char *name, int numinq, int numoutq,
                        const struct ldp_interface_settings *settings);

int ldp_dpdk_mac_addr(int portid, void *mac);

int ldp_dpdk_set_mac_addr(int portid, const void *mac);

int ldp_dpdk_promisc_mode_set(int portid, int on);

int ldp_dpdk_promisc_mode_get(int portid);

int ldp_dpdk_allmulti_set(int portid, int on);

int ldp_dpdk_allmulti_get(int portid);

#endif
