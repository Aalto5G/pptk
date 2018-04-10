#ifndef _LDP_PCAP_H_
#define _LDP_PCAP_H_

#include "ldp.h"

struct ldp_interface *
ldp_interface_open_pcap(const char *name, int numinq, int numoutq,
                        const struct ldp_interface_settings *settings);

#endif
