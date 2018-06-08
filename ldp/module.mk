LDP_SRC_LIB := ldp.c ldpnull.c linkcommon.c ldppcap.c
ifeq ($(WITH_NETMAP),yes)
LDP_SRC_LIB += ldpnetmap.c
endif
ifeq ($(WITH_DPDK),yes)
LDP_SRC_LIB += ldpdpdk.c
endif
ifeq ($(WITH_ODP),yes)
LDP_SRC_LIB += ldpodp.c
endif
LDP_SRC := $(LDP_SRC_LIB) testldp.c testrss.c ldpfwd.c ldpfwdmt.c ldptunnel.c ldprecv.c ldpreplay.c ldpswitch.c ldprecvmt.c ldpsend.c ldpsendmt.c
ifeq ($(WITH_LUA),yes)
LDP_SRC += lualdp.c
endif

LDP_SRC_LIB := $(patsubst %,$(DIRLDP)/%,$(LDP_SRC_LIB))
LDP_SRC := $(patsubst %,$(DIRLDP)/%,$(LDP_SRC))

LDP_OBJ_LIB := $(patsubst %.c,%.o,$(LDP_SRC_LIB))
LDP_OBJ := $(patsubst %.c,%.o,$(LDP_SRC))

LDP_DEP_LIB := $(patsubst %.c,%.d,$(LDP_SRC_LIB))
LDP_DEP := $(patsubst %.c,%.d,$(LDP_SRC))

CFLAGS_LDP := -I$(DIRMISC) -I$(DIRMYPCAP) -I$(DIRDYNARR) -I$(DIRHASHTABLE) -I$(DIRHASHLIST) -I$(DIRLINKEDLIST) -I$(DIRIPHDR)
MAKEFILES_LDP := $(DIRLDP)/module.mk

LIBS_LDP := $(DIRMYPCAP)/libmypcap.a $(DIRDYNARR)/libdynarr.a $(DIRIPHDR)/libiphdr.a $(DIRMISC)/libmisc.a $(DIRLOG)/liblog.a

.PHONY: LDP clean_LDP distclean_LDP unit_LDP $(LCLDP) clean_$(LCLDP) distclean_$(LCLDP) unit_$(LCLDP)

$(LCLDP): LDP
clean_$(LCLDP): clean_LDP
distclean_$(LCLDP): distclean_LDP
unit_$(LCLDP): unit_LDP

LDP: $(DIRLDP)/libldp.a $(DIRLDP)/libldp.so $(DIRLDP)/testldp $(DIRLDP)/testrss $(DIRLDP)/ldpfwd $(DIRLDP)/ldpfwdmt $(DIRLDP)/ldptunnel $(DIRLDP)/ldprecv $(DIRLDP)/ldpreplay $(DIRLDP)/ldpswitch $(DIRLDP)/ldprecvmt $(DIRLDP)/ldpsend $(DIRLDP)/ldpsendmt

ifeq ($(WITH_LUA),yes)
LDP: $(DIRLDP)/lualdp.so
endif

ifeq ($(WITH_NETMAP),yes)
CFLAGS_LDP += -I$(NETMAP_INCDIR) -DWITH_NETMAP
endif
ifeq ($(WITH_LUA),yes)
CFLAGS_LDP += -I$(LUA_INCDIR) -DWITH_LUA
LDFLAGS_LDP += $(LIBS_LUA)
endif
ifeq ($(WITH_ODP),yes)
CFLAGS_LDP += -I$(ODP_DIR)/include -DWITH_ODP
LDFLAGS_LDP += $(ODP_DIR)/lib/libodp-linux.a
LDFLAGS_LDP += $(LIBS_ODPDEP)
LDFLAGS_LDP += -lrt -ldl
LDFLAGS_LDP_DYN += -L$(ODP_DIR)/lib
LDFLAGS_LDP_DYN += -Wl,-rpath,$(ODP_DIR)/lib
LDFLAGS_LDP_DYN += -lodp-linux
endif
ifeq ($(WITH_DPDK),yes)
CFLAGS_LDP += -I$(DPDK_INCDIR) -DWITH_DPDK
CFLAGS_LDP += -msse4.2
LDFLAGS_LDP += -L$(DPDK_LIBDIR)
LDFLAGS_LDP += -Wl,--whole-archive
LDFLAGS_LDP += $(DPDK_LIBDIR)/libdpdk.a
LDFLAGS_LDP += -Wl,--no-whole-archive
LDFLAGS_LDP += -lm
LDFLAGS_LDP += /usr/lib/x86_64-linux-gnu/libnuma.a
LDFLAGS_LDP += /usr/lib/x86_64-linux-gnu/libpcap.a
LDFLAGS_LDP_DYN += -L$(DPDK_LIBDIR)
LDFLAGS_LDP_DYN += -Wl,--whole-archive
LDFLAGS_LDP_DYN += -ldpdk
LDFLAGS_LDP_DYN += -Wl,--no-whole-archive
LDFLAGS_LDP_DYN += -lnuma
LDFLAGS_LDP_DYN += -lpcap
LDFLAGS_LDP_DYN += -lm
endif

unit_LDP:
	@true

$(DIRLDP)/libldp.a: $(LDP_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRLDP)/lualdp.so: $(DIRLDP)/libldp.a $(DIRMYPCAP)/libmypcap.a $(DIRDYNARR)/libdynarr.a
	$(CC) -shared -fPIC -o $(DIRLDP)/lualdp.so $(DIRLDP)/lualdp.o -Wl,--whole-archive $(DIRLDP)/libldp.a -Wl,--no-whole-archive $(DIRMYPCAP)/libmypcap.a $(DIRDYNARR)/libdynarr.a $(LDFLAGS_LDP_DYN) -lc

$(DIRLDP)/libldp.so: $(DIRLDP)/libldp.a $(DIRMYPCAP)/libmypcap.a $(DIRDYNARR)/libdynarr.a
	$(CC) -shared -fPIC -o $(DIRLDP)/libldp.so -Wl,--whole-archive $(DIRLDP)/libldp.a -Wl,--no-whole-archive $(DIRMYPCAP)/libmypcap.a $(DIRDYNARR)/libdynarr.a $(LDFLAGS_LDP_DYN) -lc

$(DIRLDP)/testldp: $(DIRLDP)/testldp.o $(DIRLDP)/libldp.a $(LIBS_LDP) $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_LDP) $(LDFLAGS_LDP) -lpthread -ldl

$(DIRLDP)/testrss: $(DIRLDP)/testrss.o $(DIRLDP)/libldp.a $(LIBS_LDP) $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_LDP) $(LDFLAGS_LDP) -lpthread -ldl

$(DIRLDP)/ldpfwd: $(DIRLDP)/ldpfwd.o $(DIRLDP)/libldp.a $(LIBS_LDP) $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_LDP) $(LDFLAGS_LDP) -lpthread -ldl

$(DIRLDP)/ldptunnel: $(DIRLDP)/ldptunnel.o $(DIRLDP)/libldp.a $(LIBS_LDP) $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_LDP) $(LDFLAGS_LDP) -lpthread -ldl

$(DIRLDP)/ldpfwdmt: $(DIRLDP)/ldpfwdmt.o $(DIRLDP)/libldp.a $(LIBS_LDP) $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_LDP) $(LDFLAGS_LDP) -lpthread -ldl

$(DIRLDP)/ldprecv: $(DIRLDP)/ldprecv.o $(DIRLDP)/libldp.a $(LIBS_LDP) $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_LDP) $(LDFLAGS_LDP) -lpthread -ldl

$(DIRLDP)/ldprecvmt: $(DIRLDP)/ldprecvmt.o $(DIRLDP)/libldp.a $(LIBS_LDP) $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_LDP) $(LDFLAGS_LDP) -lpthread -ldl

$(DIRLDP)/ldpsend: $(DIRLDP)/ldpsend.o $(DIRLDP)/libldp.a $(LIBS_LDP) $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_LDP) $(LDFLAGS_LDP) -lpthread -ldl

$(DIRLDP)/ldpsendmt: $(DIRLDP)/ldpsendmt.o $(DIRLDP)/libldp.a $(LIBS_LDP) $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_LDP) $(LDFLAGS_LDP) -lpthread -ldl

$(DIRLDP)/ldpreplay: $(DIRLDP)/ldpreplay.o $(DIRLDP)/libldp.a $(LIBS_LDP) $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_LDP) $(LDFLAGS_LDP) -lpthread -ldl

$(DIRLDP)/ldpswitch: $(DIRLDP)/ldpswitch.o $(DIRLDP)/libldp.a $(LIBS_LDP) $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_LDP) $(LDFLAGS_LDP) -lpthread -ldl

$(LDP_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_LDP)
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c $(CFLAGS_LDP)

$(LDP_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_LDP)

clean_LDP:
	rm -f $(LDP_OBJ) $(LDP_DEP)

distclean_LDP: clean_LDP
	rm -f $(DIRLDP)/libldp.so $(DIRLDP)/libldp.a $(DIRLDP)/testldp $(DIRLDP)/testrss $(DIRLDP)/ldpfwd $(DIRLDP)/ldpfwdmt $(DIRLDP)/ldptunnel $(DIRLDP)/ldprecv $(DIRLDP)/ldpreplay $(DIRLDP)/ldpswitch $(DIRLDP)/ldprecvmt $(DIRLDP)/ldpsend $(DIRLDP)/ldpsendmt

-include $(DIRLDP)/*.d
