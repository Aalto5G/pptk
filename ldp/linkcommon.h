#ifndef _LINKCOMMON_H_
#define _LINKCOMMON_H_

int ldp_set_promisc_mode(int sockfd, const char *ifname, int on);

int ldp_link_status(int sockfd, const char *ifname);

int ldp_link_wait(int sockfd, const char *ifname);

#endif
