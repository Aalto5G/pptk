LDP_SRC_LIB := ldp.c ldpnull.c linkcommon.c
ifeq ($(WITH_NETMAP),yes)
LDP_SRC_LIB += ldpnetmap.c
endif
ifeq ($(WITH_DPDK),yes)
LDP_SRC_LIB += ldpdpdk.c
endif
LDP_SRC := $(LDP_SRC_LIB) testldp.c ldpfwd.c

LDP_SRC_LIB := $(patsubst %,$(DIRLDP)/%,$(LDP_SRC_LIB))
LDP_SRC := $(patsubst %,$(DIRLDP)/%,$(LDP_SRC))

LDP_OBJ_LIB := $(patsubst %.c,%.o,$(LDP_SRC_LIB))
LDP_OBJ := $(patsubst %.c,%.o,$(LDP_SRC))

LDP_DEP_LIB := $(patsubst %.c,%.d,$(LDP_SRC_LIB))
LDP_DEP := $(patsubst %.c,%.d,$(LDP_SRC))

CFLAGS_LDP := -I$(DIRPACKET) -I$(DIRLINKEDLIST) -I$(DIRIPHDR) -I$(DIRMISC) -I$(DIRLOG) -I$(DIRHASHTABLE) -I$(DIRHASHLIST) -I$(DIRPORTS) -I$(DIRALLOC) -I$(DIRDYNARR) -I$(DIRMYPCAP) -I$(DIRLOG)

MAKEFILES_LDP := $(DIRLDP)/module.mk

LIBS_LDP := $(DIRALLOC)/liballoc.a $(DIRIPHDR)/libiphdr.a $(DIRLOG)/liblog.a $(DIRPORTS)/libports.a $(DIRHASHTABLE)/libhashtable.a $(DIRHASHLIST)/libhashlist.a $(DIRMYPCAP)/libmypcap.a $(DIRDYNARR)/libdynarr.a

.PHONY: LDP clean_LDP distclean_LDP unit_LDP $(LCLDP) clean_$(LCLDP) distclean_$(LCLDP) unit_$(LCLDP)

$(LCLDP): LDP
clean_$(LCLDP): clean_LDP
distclean_$(LCLDP): distclean_LDP
unit_$(LCLDP): unit_LDP

LDP: $(DIRLDP)/libldp.a $(DIRLDP)/testldp $(DIRLDP)/ldpfwd

ifeq ($(WITH_NETMAP),yes)
CFLAGS_LDP += -I$(NETMAP_INCDIR) -DWITH_NETMAP
endif
ifeq ($(WITH_DPDK),yes)
CFLAGS_LDP += -I$(DPDK_INCDIR) -DWITH_DPDK
CFLAGS_LDP += -msse4.2
#CFLAGS_LDP += -L$(DPDK_LIBDIR)
LDFLAGS_LDP += -Wl,--whole-archive
LDFLAGS_LDP += $(DPDK_LIBDIR)/librte_ethdev.a
LDFLAGS_LDP += $(DPDK_LIBDIR)/librte_pmd_pcap.a
LDFLAGS_LDP += $(DPDK_LIBDIR)/librte_mbuf.a
LDFLAGS_LDP += $(DPDK_LIBDIR)/librte_mempool.a
LDFLAGS_LDP += $(DPDK_LIBDIR)/librte_mempool_ring.a
LDFLAGS_LDP += $(DPDK_LIBDIR)/librte_ring.a
LDFLAGS_LDP += $(DPDK_LIBDIR)/librte_kvargs.a
LDFLAGS_LDP += $(DPDK_LIBDIR)/librte_eal.a
LDFLAGS_LDP += -Wl,--no-whole-archive
LDFLAGS_LDP += /usr/lib/x86_64-linux-gnu/libnuma.a
LDFLAGS_LDP += /usr/lib/x86_64-linux-gnu/libpcap.a
endif

unit_LDP:
	@true

$(DIRLDP)/libldp.a: $(LDP_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRLDP)/testldp: $(DIRLDP)/testldp.o $(DIRLDP)/libldp.a $(LIBS_LDP) $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_LDP) $(LDFLAGS_LDP) -lpthread -ldl

$(DIRLDP)/ldpfwd: $(DIRLDP)/ldpfwd.o $(DIRLDP)/libldp.a $(LIBS_LDP) $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_LDP) $(LDFLAGS_LDP) -lpthread -ldl

$(LDP_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_LDP)
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c $(CFLAGS_LDP)

$(LDP_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_LDP)

clean_LDP:
	rm -f $(LDP_OBJ) $(LDP_DEP)

distclean_LDP: clean_LDP
	rm -f $(DIRLDP)/libldp.a $(DIRLDP)/testldp $(DIRLDP)/ldpfwd

-include $(DIRLDP)/*.d
