NETMAP_SRC_LIB :=
ifeq ($(WITH_NETMAP),yes)
NETMAP_SRC_LIB := netmapcommon.c
endif
NETMAP_SRC := $(NETMAP_SRC_LIB) netmapfwd.c netmaprecv.c netmapreplay.c

NETMAP_SRC_LIB := $(patsubst %,$(DIRNETMAP)/%,$(NETMAP_SRC_LIB))
NETMAP_SRC := $(patsubst %,$(DIRNETMAP)/%,$(NETMAP_SRC))

NETMAP_OBJ_LIB := $(patsubst %.c,%.o,$(NETMAP_SRC_LIB))
NETMAP_OBJ := $(patsubst %.c,%.o,$(NETMAP_SRC))

NETMAP_DEP_LIB := $(patsubst %.c,%.d,$(NETMAP_SRC_LIB))
NETMAP_DEP := $(patsubst %.c,%.d,$(NETMAP_SRC))

CFLAGS_NETMAP := -I$(DIRPACKET) -I$(DIRLINKEDLIST) -I$(DIRIPHDR) -I$(DIRMISC) -I$(DIRLOG) -I$(DIRHASHTABLE) -I$(DIRHASHLIST) -I$(DIRARP) -I$(DIRPORTS) -I$(DIRALLOC) -I$(DIRDYNARR) -I$(DIRMYPCAP)

MAKEFILES_NETMAP := $(DIRNETMAP)/module.mk

LIBS_NETMAP := $(DIRALLOC)/liballoc.a $(DIRIPHDR)/libiphdr.a $(DIRLOG)/liblog.a $(DIRARP)/libarp.a $(DIRPORTS)/libports.a $(DIRHASHTABLE)/libhashtable.a $(DIRHASHLIST)/libhashlist.a $(DIRMYPCAP)/libmypcap.a $(DIRDYNARR)/libdynarr.a

.PHONY: NETMAP clean_NETMAP distclean_NETMAP unit_NETMAP $(LCNETMAP) clean_$(LCNETMAP) distclean_$(LCNETMAP) unit_$(LCNETMAP)

$(LCNETMAP): NETMAP
clean_$(LCNETMAP): clean_NETMAP
distclean_$(LCNETMAP): distclean_NETMAP
unit_$(LCNETMAP): unit_NETMAP

NETMAP: $(DIRNETMAP)/libnetmap.a

ifeq ($(WITH_NETMAP),yes)
NETMAP: $(DIRNETMAP)/netmapfwd $(DIRNETMAP)/netmaprecv $(DIRNETMAP)/netmapreplay
CFLAGS_NETMAP += -I$(NETMAP_INCDIR)
endif

unit_NETMAP:
	@true

$(DIRNETMAP)/libnetmap.a: $(NETMAP_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_NETMAP)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRNETMAP)/netmapfwd: $(DIRNETMAP)/netmapfwd.o $(DIRNETMAP)/libnetmap.a $(LIBS_NETMAP) $(MAKEFILES_COMMON) $(MAKEFILES_NETMAP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_NETMAP) -lpthread

$(DIRNETMAP)/netmaprecv: $(DIRNETMAP)/netmaprecv.o $(DIRNETMAP)/libnetmap.a $(LIBS_NETMAP) $(MAKEFILES_COMMON) $(MAKEFILES_NETMAP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_NETMAP) -lpthread

$(DIRNETMAP)/netmapreplay: $(DIRNETMAP)/netmapreplay.o $(DIRNETMAP)/libnetmap.a $(LIBS_NETMAP) $(MAKEFILES_COMMON) $(MAKEFILES_NETMAP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_NETMAP) -lpthread

$(NETMAP_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_NETMAP)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_NETMAP)

$(NETMAP_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_NETMAP)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_NETMAP)

clean_NETMAP:
	rm -f $(NETMAP_OBJ) $(NETMAP_DEP)

distclean_NETMAP: clean_NETMAP
	rm -f $(DIRNETMAP)/libnetmap.a $(DIRNETMAP)/netmapperf $(DIRNETMAP)/netmapfwd $(DIRNETMAP)/netmaprecv

-include $(DIRNETMAP)/*.d
