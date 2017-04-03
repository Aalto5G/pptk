CC := cc
#CC := clang

.SUFFIXES:

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

DIRLINKEDLIST := linkedlist
LCLINKEDLIST := linkedlist
MODULES += LINKEDLIST

DIRLOG := log
LCLOG := log
MODULES += LOG

DIRIPHDR := iphdr
LCIPHDR := iphdr
MODULES += IPHDR

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

CFLAGS := -g -O2 -Wall -Werror

.PHONY: all clean distclean unit

all: $(MODULES)
clean: $(patsubst %,clean_%,$(MODULES))
distclean: $(patsubst %,distclean_%,$(MODULES))
unit: $(patsubst %,unit_%,$(MODULES))

MAKEFILES_COMMON := Makefile opts.mk

WITH_NETMAP=no
NETMAP_INCDIR=
include opts.mk

$(foreach module,$(MODULES),$(eval \
    include $(DIR$(module))/module.mk))

opts.mk:
	touch opts.mk
