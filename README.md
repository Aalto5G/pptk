# PPTK

PPTK is a packet processing toolkit. Originally it was only for processing. It
was not for reception or transmission, although it was intended to be used with
netmap. There is however no tight connection to netmap in PPTK, so PPTK can be
used along with regular raw sockets as well.

However, now there's LDP (L Data Plane) included in PPTK as well. Read this
document fully for more information.

PPTK aims to do things correctly. So, for example, fragment reassembly supports
an arbitrary number of fragments per packet and handles overlapping fragments
properly. This is in contrast to DPDK, that supports only 4 fragments per
packet and is otherwise highly suspicious.

Whenever feasible, the already provided implementations are used. So, we aren't
interested in creating new implementations of spin locks (mutexes provided
already in Linux are fine) or read-write locks (the Linux read-write locks
don't have writer starvation by readers). This is in contrast to DPDK or
OpenDataPlane.

PPTK is very librarylike. You can easily pick only some parts from PPTK and
provide implementation for the rest yourself. There is no need to call early
from main a function to initialize PPTK, like you must do for DPDK or
OpenDataPlane.

There is no requirement of configuring huge pages. There is no requirement for
root permissions unless doing raw packet I/O.

If there are multiple ways of doing things, many ways are supported. So for
example timers can be done by AVL trees, red-black trees, skip lists or link
heaps. Also, timer wheel is provided but it is expected that link heap is
superior. Similarly, IP fragment reassembly supports many ways. This is in
contrast to DPDK and OpenDataPlane, of which DPDK has only skip list timers,
OpenDataPlane has only timer wheel, and both have only one way of reassembling
IP datagrams (in ODP, the datagram reassembly is just an example).

Please note that some code is under a peculiar license. So, for example the
skip list based timer code taken from DPDK is under BSD license and the Linux
IP fragment reassembly code is under GPLv2.

# LDP

LDP is L Data Plane. What does the L mean, then? Well, everyone can interpret
it in a different way. It can mean Linux, as LDP will probably never support
any other platform. It can also mean lightweight, as LDP is much lighterweight
than other data plane wrappers like OpenDataPlane (ODP).

LDP aims to mainly offer a lighterweight alternative to ODP. Packet reception
does not require allocating memory in LDP, which is the main benefit of LDP
over ODP. Instead, packets are invalidated the next time a packet burst is
received or the next time information is synchronized with the kernel by
polling on the file descriptor. This means the packet input areas belonging to
the network interface card in netmap can be used.

# Prerequisites

Needless to say, compiler tools and GNU make must be available. To actually
communicate with real network interfaces, you also need netmap, but more on
that later.

# Git history

The git history has been edited to remove some proprietary code. Thus,
historical versions of the repository may not compile correctly.

# Compilation

To compile, type `make -j4` where the number after `-j` is the number of cores.

To run unit tests, run `make unit`. Note that using `-j` with `make unit` is
not recommended.

# Netmap support

To compile with netmap support, edit the file `opts.mk` (generated as empty
file automatically after successful `make`), and add the lines:

```
WITH_NETMAP=yes
NETMAP_INCDIR=/home/YOURUSERNAME/netmap/sys
```

But before this, you need to clone netmap:

```
cd /home/YOURUSERNAME
git clone https://github.com/luigirizzo/netmap
cd netmap
./configure --no-drivers
make
insmod ./netmap.ko
```

Successfully compiling netmap requires that you have your kernel headers
installed.

# Netmap drivers

If you want higher performance, you can compile netmap with drivers:

```
cd /home/YOURUSERNAME
rm -rf netmap
git clone https://github.com/luigirizzo/netmap
cd netmap
./configure
make
rmmod netmap
rmmod ixgbe
rmmod i40e
insmod ./netmap.ko
insmod ./ixgbe-5.0.4/src/ixgbe.ko
insmod ./i40e-2.0.19/src/i40e.ko
```

Adjust paths as needed to have the correct version of the driver.

# Netmap testing

Then, after netmap is installed, compile with `make -j4` as usual.

# Netmap with full kernel sources

Some netmap drivers require full kernel sources. On Ubuntu 16.04 LTS, they
can be installed in the following way: first, uncomment deb-src lines in
`/etc/apt/sources.list`. Then, type these commands:

```
cd /home/YOURUSERNAME
apt-get update
apt-get source linux-image-$(uname -r)
rm -rf netmap
git clone https://github.com/luigirizzo/netmap
cd netmap
./configure --kernel-sources=/home/WHATEVER/linux-hwe-4.8.0
rmmod netmap
insmod ./netmap.ko
```

Then, you may load for example netmap specific veth driver:

```
cd /home/YOURUSERNAME/netmap
rmmod veth
insmod ./veth.ko
```
