CC := cc
#CC := clang

.SUFFIXES:

DIRTCPREASS := tcpreass
LCTCPREASS := tcpreass
MODULES += TCPREASS

DIRDYNARR := dynarr
LCDYNARR := dynarr
MODULES += DYNARR

DIRMISC := misc
LCMISC := misc
MODULES += MISC

DIRHASHLIST := hashlist
LCHASHLIST := hashlist
MODULES += HASHLIST

DIRHASHTABLE := hashtable
LCHASHTABLE := hashtable
MODULES += HASHTABLE

DIRRBTREE := rbtree
LCRBTREE := rbtree
MODULES += RBTREE

DIRAVLTREE := avltree
LCAVLTREE := avltree
MODULES += AVLTREE

DIRTIMERRB := timerrb
LCTIMERRB := timerrb
MODULES += TIMERRB

DIRTIMERAVL := timeravl
LCTIMERAVL := timeravl
MODULES += TIMERAVL

DIRLINKEDLIST := linkedlist
LCLINKEDLIST := linkedlist
MODULES += LINKEDLIST

DIRTIMERLINKHEAP := timerlinkheap
LCTIMERLINKHEAP := timerlinkheap
MODULES += TIMERLINKHEAP

DIRTIMERSKIPLIST := timerskiplist
LCTIMERSKIPLIST := timerskiplist
MODULES += TIMERSKIPLIST

DIRTIMERWHEEL := timerwheel
LCTIMERWHEEL := timerwheel
MODULES += TIMERWHEEL

DIRLOG := log
LCLOG := log
MODULES += LOG

DIRIPHDR := iphdr
LCIPHDR := iphdr
MODULES += IPHDR

DIRIPFRAG := ipfrag
LCIPFRAG := ipfrag
MODULES += IPFRAG

DIRPACKET := packet
LCPACKET := packet
MODULES += PACKET

DIRARP := arp
LCARP := arp
MODULES += ARP

DIRPORTS := ports
LCPORTS := ports
MODULES += PORTS

DIRQUEUE := queue
LCQUEUE := queue
MODULES += QUEUE

DIRALLOC := alloc
LCALLOC := alloc
MODULES += ALLOC

DIRRANDOM := random
LCRANDOM := random
MODULES += RANDOM

DIRDATABUF := databuf
LCDATABUF := databuf
MODULES += DATABUF

DIRTUNTAP := tuntap
LCTUNTAP := tuntap
MODULES += TUNTAP

DIRPORTLIST := portlist
LCPORTLIST := portlist
MODULES += PORTLIST

DIRNETMAP := netmap
LCNETMAP := netmap
MODULES += NETMAP

DIRIPHASH := iphash
LCIPHASH := iphash
MODULES += IPHASH

DIRMYPCAP := mypcap
LCMYPCAP := mypcap
MODULES += MYPCAP

DIRLDP := ldp
LCLDP := ldp
MODULES += LDP

DIRLIBPPTK := libpptk
LCLIBPPTK := libpptk
MODULES += LIBPPTK

CFLAGS := -g -O2 -Wall -Wextra -Wsign-conversion -Wno-missing-field-initializers -Wno-unused-parameter -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith -std=gnu11 -fPIC

.PHONY: all clean distclean unit

all: $(MODULES)
clean: $(patsubst %,clean_%,$(MODULES))
distclean: $(patsubst %,distclean_%,$(MODULES))
unit: $(patsubst %,unit_%,$(MODULES))

MAKEFILES_COMMON := Makefile opts.mk

WITH_NETMAP=no
WITH_WERROR=no
NETMAP_INCDIR=
WITH_ODP=no
ODP_DIR=/usr/local
WITH_DPDK=no
DDPK_INCDIR=
DPDK_LIBDIR=
LIBS_ODPDEP=/usr/lib/x86_64-linux-gnu/libssl.a /usr/lib/x86_64-linux-gnu/libcrypto.a
include opts.mk

ifeq ($(WITH_WERROR),yes)
CFLAGS := $(CFLAGS) -Werror
endif

$(foreach module,$(MODULES),$(eval \
    include $(DIR$(module))/module.mk))

opts.mk:
	touch opts.mk
