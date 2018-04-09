#ifndef _LDP_NETMAP_H_
#define _LDP_NETMAP_H_

#include "ldp.h"

struct ldp_interface *
ldp_interface_open_netmap(const char *name, int numinq, int numoutq,
                          const struct ldp_interface_settings *settings);

#endif
