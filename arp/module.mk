ARP_SRC_LIB := arp.c
ARP_SRC := $(ARP_SRC_LIB)

ARP_SRC_LIB := $(patsubst %,$(DIRARP)/%,$(ARP_SRC_LIB))
ARP_SRC := $(patsubst %,$(DIRARP)/%,$(ARP_SRC))

ARP_OBJ_LIB := $(patsubst %.c,%.o,$(ARP_SRC_LIB))
ARP_OBJ := $(patsubst %.c,%.o,$(ARP_SRC))

ARP_DEP_LIB := $(patsubst %.c,%.d,$(ARP_SRC_LIB))
ARP_DEP := $(patsubst %.c,%.d,$(ARP_SRC))

CFLAGS_ARP := -I$(DIRHASHLIST) -I$(DIRMISC) -I$(DIRPACKET) -I$(DIRLINKEDLIST) -I$(DIRHASHTABLE) -I$(DIRPORTS) -I$(DIRIPHDR) -I$(DIRALLOC)

MAKEFILES_ARP := $(DIRARP)/module.mk

.PHONY: ARP clean_ARP distclean_ARP unit_ARP $(LCARP) clean_$(LCARP) distclean_$(LCARP) unit_$(LCARP)

$(LCARP): ARP
clean_$(LCARP): clean_ARP
distclean_$(LCARP): distclean_ARP
unit_$(LCARP): unit_ARP

ARP: $(DIRARP)/libarp.a

unit_ARP:
	@true

$(DIRARP)/libarp.a: $(ARP_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_ARP)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(ARP_OBJ): %.o: %.c $($*.d) $(MAKEFILES_COMMON) $(MAKEFILES_ARP)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_ARP)

$(ARP_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_ARP)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_ARP)

clean_ARP:
	rm -f $(ARP_OBJ) $(ARP_DEP)

distclean_ARP: clean_ARP
	rm -f $(DIRARP)/libarp.a

-include $(DIRARP)/*.d
