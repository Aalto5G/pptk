LDP_SRC_LIB :=
ifeq ($(WITH_NETMAP),yes)
LDP_SRC_LIB += ldp.c
endif
LDP_SRC := $(LDP_SRC_LIB)
ifeq ($(WITH_NETMAP),yes)
LDP_SRC += testldp.c
endif

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

LDP: $(DIRLDP)/libldp.a

ifeq ($(WITH_NETMAP),yes)
LDP: $(DIRLDP)/testldp
CFLAGS_LDP += -I$(NETMAP_INCDIR)
endif

unit_LDP:
	@true

$(DIRLDP)/libldp.a: $(LDP_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRLDP)/testldp: $(DIRLDP)/testldp.o $(DIRLDP)/libldp.a $(LIBS_LDP) $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_LDP) -lpthread

$(LDP_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_LDP)
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c $(CFLAGS_LDP)

$(LDP_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_LDP)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_LDP)

clean_LDP:
	rm -f $(LDP_OBJ) $(LDP_DEP)

distclean_LDP: clean_LDP
	rm -f $(DIRLDP)/libldp.a $(DIRLDP)/testldp

-include $(DIRLDP)/*.d
