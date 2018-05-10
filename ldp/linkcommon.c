#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/if_arp.h>
#include "siphash.h"
#include "linkcommon.h"
#include "ldpdpdk.h"

int ldp_set_mtu(int sockfd, const char *ifname, uint16_t mtu)
{
  struct ifreq ifr;
  long portid;
  char *endptr;
  portid = strtol(ifname, &endptr, 10);
  if (*ifname != '\0' && *endptr == '\0' && portid >= 0 && portid <= INT_MAX)
  {
    return -ENOTSUP; // DPDK, not supported when interface is running
  }

  memset(&ifr, 0, sizeof(ifr));
  if (strncmp(ifname, "null:", 5) == 0)
  {
    return 0;
  }
  if (strncmp(ifname, "odp:", 4) == 0)
  {
    return -ENOTSUP;
  }
  if (strncmp(ifname, "pcap:", 5) == 0)
  {
    return -ENOTSUP;
  }
  if (strncmp(ifname, "vale", 4) == 0)
  {
    return 0;
  }
  if (strncmp(ifname, "netmap:", 7) == 0)
  {
    ifname = ifname + 7;
  }
  snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
  ifr.ifr_mtu = mtu;
  if (ioctl(sockfd, SIOCSIFMTU, &ifr) < 0)
  {
    return -1;
  }
  return 0;
}

int ldp_get_mtu(int sockfd, const char *ifname)
{
  struct ifreq ifr;
  long portid;
  char *endptr;
  portid = strtol(ifname, &endptr, 10);
  if (*ifname != '\0' && *endptr == '\0' && portid >= 0 && portid <= INT_MAX)
  {
#if WITH_DPDK
    return ldp_dpdk_mtu_get(portid);
#else
    return -ENOTSUP; // DPDK
#endif
  }

  memset(&ifr, 0, sizeof(ifr));
  if (strncmp(ifname, "null:", 5) == 0)
  {
    return -ENOTSUP;
  }
  if (strncmp(ifname, "odp:", 4) == 0)
  {
    return -ENOTSUP;
  }
  if (strncmp(ifname, "pcap:", 5) == 0)
  {
    return -ENOTSUP;
  }
  if (strncmp(ifname, "vale", 4) == 0)
  {
    return -ENOTSUP;
  }
  if (strncmp(ifname, "netmap:", 7) == 0)
  {
    ifname = ifname + 7;
  }
  snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
  if (ioctl(sockfd, SIOCGIFMTU, &ifr) < 0)
  {
    return -1;
  }
  return ifr.ifr_mtu;
}

int ldp_set_promisc_mode(int sockfd, const char *ifname, int on)
{
  struct ifreq ifr;
  long portid;
  char *endptr;
  portid = strtol(ifname, &endptr, 10);
  if (*ifname != '\0' && *endptr == '\0' && portid >= 0 && portid <= INT_MAX)
  {
#if WITH_DPDK
    return ldp_dpdk_promisc_mode_set(portid, on);
#else
    return -ENOTSUP; // DPDK
#endif
  }

  memset(&ifr, 0, sizeof(ifr));
  if (strncmp(ifname, "null:", 5) == 0)
  {
    return 0;
  }
  if (strncmp(ifname, "pcap:", 5) == 0)
  {
    return 0;
  }
  if (strncmp(ifname, "vale", 4) == 0)
  {
    return 0;
  }
  if (strncmp(ifname, "netmap:", 7) == 0)
  {
    ifname = ifname + 7;
  }
  if (on)
  {
    snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    ifr.ifr_flags &= ~IFF_PROMISC;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    ifr.ifr_flags |= IFF_PROMISC;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
  }
  else
  {
    snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    ifr.ifr_flags |= IFF_PROMISC;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    ifr.ifr_flags &= ~IFF_PROMISC;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
  }
  return 0;
}

int ldp_get_promisc_mode(int sockfd, const char *ifname)
{
  struct ifreq ifr;
  long portid;
  char *endptr;
  portid = strtol(ifname, &endptr, 10);
  if (*ifname != '\0' && *endptr == '\0' && portid >= 0 && portid <= INT_MAX)
  {
#if WITH_DPDK
    return ldp_dpdk_promisc_mode_get(portid);
#else
    return -ENOTSUP; // DPDK
#endif
  }

  memset(&ifr, 0, sizeof(ifr));
  if (strncmp(ifname, "null:", 5) == 0)
  {
    return 0;
  }
  if (strncmp(ifname, "pcap:", 5) == 0)
  {
    return 1;
  }
  if (strncmp(ifname, "vale", 4) == 0)
  {
    return 0;
  }
  if (strncmp(ifname, "netmap:", 7) == 0)
  {
    ifname = ifname + 7;
  }
  snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
  if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
  {
    return -1;
  }
  return !!(ifr.ifr_flags & IFF_PROMISC);
}

int ldp_set_allmulti(int sockfd, const char *ifname, int on)
{
  struct ifreq ifr;
  long portid;
  char *endptr;
  portid = strtol(ifname, &endptr, 10);
  if (*ifname != '\0' && *endptr == '\0' && portid >= 0 && portid <= INT_MAX)
  {
#if WITH_DPDK
    return ldp_dpdk_allmulti_set(portid, on);
#else
    return -ENOTSUP; // DPDK
#endif
  }

  memset(&ifr, 0, sizeof(ifr));
  if (strncmp(ifname, "null:", 5) == 0)
  {
    return 0;
  }
  if (strncmp(ifname, "pcap:", 5) == 0)
  {
    return 0;
  }
  if (strncmp(ifname, "odp:", 4) == 0)
  {
    return -ENOTSUP;
  }
  if (strncmp(ifname, "vale", 4) == 0)
  {
    return 0;
  }
  if (strncmp(ifname, "netmap:", 7) == 0)
  {
    ifname = ifname + 7;
  }
  if (on)
  {
    snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    ifr.ifr_flags &= ~IFF_ALLMULTI;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    ifr.ifr_flags |= IFF_ALLMULTI;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
  }
  else
  {
    snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    ifr.ifr_flags |= IFF_ALLMULTI;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
    ifr.ifr_flags &= ~IFF_ALLMULTI;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
    {
      return -1;
    }
  }
  return 0;
}

int ldp_get_allmulti(int sockfd, const char *ifname)
{
  struct ifreq ifr;
  long portid;
  char *endptr;
  portid = strtol(ifname, &endptr, 10);
  if (*ifname != '\0' && *endptr == '\0' && portid >= 0 && portid <= INT_MAX)
  {
#if WITH_DPDK
    return ldp_dpdk_allmulti_get(portid);
#else
    return -ENOTSUP; // DPDK
#endif
  }

  memset(&ifr, 0, sizeof(ifr));
  if (strncmp(ifname, "null:", 5) == 0)
  {
    return 0;
  }
  if (strncmp(ifname, "pcap:", 5) == 0)
  {
    return 1;
  }
  if (strncmp(ifname, "odp:", 4) == 0)
  {
    return -ENOTSUP;
  }
  if (strncmp(ifname, "vale", 4) == 0)
  {
    return 0;
  }
  if (strncmp(ifname, "netmap:", 7) == 0)
  {
    ifname = ifname + 7;
  }
  snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
  if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
  {
    return -1;
  }
  return !!(ifr.ifr_flags & IFF_ALLMULTI);
}

int ldp_link_status(int sockfd, const char *ifname)
{
  struct ifreq ifr;
  long portid;
  char *endptr;
  portid = strtol(ifname, &endptr, 10);
  if (*ifname != '\0' && *endptr == '\0' && portid >= 0 && portid <= INT_MAX)
  {
    return 1; // DPDK
  }
  memset(&ifr, 0, sizeof(ifr));
  if (strncmp(ifname, "vale", 4) == 0)
  {
    return 1;
  }
  if (strncmp(ifname, "null:", 5) == 0)
  {
    return 1;
  }
  if (strncmp(ifname, "odp:", 4) == 0)
  {
    return 1;
  }
  if (strncmp(ifname, "pcap:", 5) == 0)
  {
    return 1;
  }
  if (strncmp(ifname, "netmap:", 7) == 0)
  {
    ifname = ifname + 7;
  }
  snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
  if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
  {
    return 0;
  }
  return !!(ifr.ifr_flags & IFF_RUNNING);
}

int ldp_mac_addr(int sockfd, const char *ifname, void *mac)
{
  struct ifreq ifr;
  long portid;
  char *endptr;
  portid = strtol(ifname, &endptr, 10);
  if (*ifname != '\0' && *endptr == '\0' && portid >= 0 && portid <= INT_MAX)
  {
#if WITH_DPDK
    return ldp_dpdk_mac_addr(portid, mac);
#else
    return -ENOTSUP; // DPDK
#endif
  }
  memset(&ifr, 0, sizeof(ifr));
  if (strncmp(ifname, "vale", 4) == 0)
  {
    char hash_seed[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    uint64_t addr64 = siphash_buf(hash_seed, ifname, strlen(ifname));
    memcpy(mac, &addr64, 6);
    ((char*)mac)[0] |= 0x02;
    ((char*)mac)[0] &= ~0x01;
    return 0;
  }
  if (strncmp(ifname, "null:", 5) == 0)
  {
    char hash_seed[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    uint64_t addr64 = siphash_buf(hash_seed, ifname, strlen(ifname));
    memcpy(mac, &addr64, 6);
    ((char*)mac)[0] |= 0x02;
    ((char*)mac)[0] &= ~0x01;
    return 0;
  }
  if (strncmp(ifname, "pcap:", 5) == 0)
  {
    char hash_seed[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    uint64_t addr64 = siphash_buf(hash_seed, ifname, strlen(ifname));
    memcpy(mac, &addr64, 6);
    ((char*)mac)[0] |= 0x02;
    ((char*)mac)[0] &= ~0x01;
    return 0;
  }
  if (strncmp(ifname, "netmap:", 7) == 0)
  {
    ifname = ifname + 7;
  }
  snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
  if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0)
  {
    if (errno > 0)
    {
      return -errno;
    }
    return -ENETDOWN;
  }
  memcpy(mac, ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);
  return 0;
}

int ldp_set_mac_addr(int sockfd, const char *ifname, const void *mac)
{
  struct ifreq ifr;
  long portid;
  char *endptr;
  portid = strtol(ifname, &endptr, 10);
  if (*ifname != '\0' && *endptr == '\0' && portid >= 0 && portid <= INT_MAX)
  {
#if WITH_DPDK
    return ldp_dpdk_set_mac_addr(portid, mac);
#else
    return -ENOTSUP; // DPDK
#endif
  }
  memset(&ifr, 0, sizeof(ifr));
  if (strncmp(ifname, "vale", 4) == 0)
  {
    return -ENOTSUP;
  }
  if (strncmp(ifname, "null:", 5) == 0)
  {
    return -ENOTSUP;
  }
  if (strncmp(ifname, "pcap:", 5) == 0)
  {
    return -ENOTSUP;
  }
  if (strncmp(ifname, "odp:", 4) == 0)
  {
    return -ENOTSUP;
  }
  if (strncmp(ifname, "netmap:", 7) == 0)
  {
    ifname = ifname + 7;
  }
  snprintf(ifr.ifr_name, IF_NAMESIZE, "%s", ifname);
  ifr.ifr_ifru.ifru_hwaddr.sa_family = ARPHRD_ETHER;
  memcpy(ifr.ifr_ifru.ifru_hwaddr.sa_data, mac, 6);
  if (ioctl(sockfd, SIOCSIFHWADDR, &ifr) < 0)
  {
    if (errno > 0)
    {
      return -errno;
    }
    return -ENETDOWN;
  }
  return 0;
}

int ldp_link_wait(int sockfd, const char *ifname)
{
  int i;
  for (i = 0; i <= 10; i++)
  {
    if (ldp_link_status(sockfd, ifname))
    {
      sleep(1); // Better safe than sorry
      return 0;
    }
    sleep(1);
  }
  return -ENETDOWN;
}
