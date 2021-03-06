# PPTK

![PPTK](pptklogo.png)

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

# Data plane wrappers

One might ask whether data plane wrappers are needed at all. After all, isn't
it possible to use either netmap or DPDK directly?

However, there are multiple packet I/O technologies. They have differing
network interface card support. Building an entire application on one means
one cannot use another.

Also, raw netmap has a poor multithreaded interface. For example, if three
threads are used on an 8-core machine, it isn't possible to simply open three
input queues with netmap. If only three input queues are opened on an 8-core
machine, the packets going to the non-opened five will be lost.

To solve this issue, there are data plane wrappers like OpenDataPlane (ODP).
However, ODP has suboptimal performance due to its interface that requires the
packet memory to be managed by ODP. This means additional memory copies are
required with technologies like netmap that want to manage the memory
internally. Such additional memory copies result in suboptimal performance.

In order to remedy the poor performance of ODP, PPTK has a data plane wrapper
called LDP. LDP supports packet I/O via sockets, netmap, DPDK, and also via
ODP. In order to improve the poor multithreaded interface of netmap, LDP
automatically checks that the number of queues in an interface is properly
configured before starting the LDP application, and also tries to make
automatic adjustments to the queue count if possible. Furthermore, offloads are
automatically adjusted.

# LDP

![LDP](ldplogo.png)

LDP is L Data Plane. What does the L mean, then? Well, everyone can interpret
it in a different way. It can mean Linux, as LDP will probably never support
any other platform. It can also mean lightweight, as LDP is much lighterweight
than other data plane wrappers like OpenDataPlane (ODP).

LDP aims to mainly offer a lighterweight alternative to ODP. Packet reception
does not require allocating memory in LDP when used with netmap, because
packets point to the NIC ring memory. Packet memory however must be explicitly
released after packets are no longer needed.

LDP supports scatter/gather output. This means that e.g. tunneling can be done
by having the packet scattered into two blocks: tunneling header and original
packet. The output memory copy then gathers the full packet. Standard iovec
structures are used for scatter/gather, meaning they are fully compatible with
writev, sendmsg and sendmmsg (the socket functionality of LDP uses sendmmsg).

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
