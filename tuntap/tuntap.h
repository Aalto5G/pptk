#ifndef _TUNTAP_H_
#define _TUNTAP_H_

#include <netinet/in.h>
#include <linux/if.h>

int tap_alloc(const char *devreq, char actual_dev[IFNAMSIZ]);

int tap_bring_up(const char *actual_dev);

#endif
