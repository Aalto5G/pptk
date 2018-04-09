#ifndef _LDP_ODP_H_
#define _LDP_ODP_H_

#include "ldp.h"

struct ldp_interface *
ldp_interface_open_odp(const char *name, int numinq, int numoutq,
                       const struct ldp_interface_settings *settings);

#endif
