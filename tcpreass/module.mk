TCPREASS_SRC_LIB := tcprbexplicit.c
TCPREASS_SRC := $(TCPREASS_SRC_LIB) tcprbexplicittest.c

TCPREASS_SRC_LIB := $(patsubst %,$(DIRTCPREASS)/%,$(TCPREASS_SRC_LIB))
TCPREASS_SRC := $(patsubst %,$(DIRTCPREASS)/%,$(TCPREASS_SRC))

TCPREASS_OBJ_LIB := $(patsubst %.c,%.o,$(TCPREASS_SRC_LIB))
TCPREASS_OBJ := $(patsubst %.c,%.o,$(TCPREASS_SRC))

TCPREASS_DEP_LIB := $(patsubst %.c,%.d,$(TCPREASS_SRC_LIB))
TCPREASS_DEP := $(patsubst %.c,%.d,$(TCPREASS_SRC))

CFLAGS_TCPREASS := -I$(DIRMISC) -I$(DIRIPHDR) -I$(DIRALLOC) -I$(DIRPACKET) -I$(DIRLINKEDLIST) -I$(DIRMYPCAP) -I$(DIRRBTREE)
LIBS_TCPREASS := $(DIRALLOC)/liballoc.a $(DIRIPHDR)/libiphdr.a $(DIRMYPCAP)/libmypcap.a $(DIRRBTREE)/librbtree.a

MAKEFILES_TCPREASS := $(DIRTCPREASS)/module.mk

.PHONY: TCPREASS clean_TCPREASS distclean_TCPREASS unit_TCPREASS $(LCTCPREASS) clean_$(LCTCPREASS) distclean_$(LCTCPREASS) unit_$(LCTCPREASS)

$(LCTCPREASS): TCPREASS
clean_$(LCTCPREASS): clean_TCPREASS
distclean_$(LCTCPREASS): distclean_TCPREASS
unit_$(LCTCPREASS): unit_TCPREASS

TCPREASS: $(DIRTCPREASS)/libtcpreass.a $(DIRTCPREASS)/tcprbexplicittest

unit_TCPREASS:
	@true

$(DIRTCPREASS)/libtcpreass.a: $(TCPREASS_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_TCPREASS)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRTCPREASS)/tcprbexplicittest: $(DIRTCPREASS)/tcprbexplicittest.o $(DIRTCPREASS)/libtcpreass.a $(LIBS_TCPREASS) $(MAKEFILES_COMMON) $(MAKEFILES_TCPREASS)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_TCPREASS)

$(TCPREASS_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_TCPREASS)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_TCPREASS)
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c $(CFLAGS_TCPREASS)

$(TCPREASS_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_TCPREASS)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_TCPREASS)

clean_TCPREASS:
	rm -f $(TCPREASS_OBJ) $(TCPREASS_DEP)

distclean_TCPREASS: clean_TCPREASS
	rm -f $(DIRTCPREASS)/libtcpreass.a $(DIRTCPREASS)/tcprbexplicittest

-include $(DIRTCPREASS)/*.d
