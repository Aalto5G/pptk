#ifndef _TUNTAP_H_
#define _TUNTAP_H_

#include <netinet/in.h>
#include <linux/if.h>

int tap_alloc(const char *devreq, char actual_dev[IFNAMSIZ]);

int tun_alloc(const char *devreq, char actual_dev[IFNAMSIZ]);

int tap_bring_up(const char *actual_dev);

int tun_bring_up_addr(const char *actual_dev, uint32_t addr, uint32_t peer);

#endif
