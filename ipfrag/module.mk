IPFRAG_SRC_LIB := ipfrag.c ipreass.c
IPFRAG_SRC := $(IPFRAG_SRC_LIB) ipfragtest.c ipreasstest.c

IPFRAG_SRC_LIB := $(patsubst %,$(DIRIPFRAG)/%,$(IPFRAG_SRC_LIB))
IPFRAG_SRC := $(patsubst %,$(DIRIPFRAG)/%,$(IPFRAG_SRC))

IPFRAG_OBJ_LIB := $(patsubst %.c,%.o,$(IPFRAG_SRC_LIB))
IPFRAG_OBJ := $(patsubst %.c,%.o,$(IPFRAG_SRC))

IPFRAG_DEP_LIB := $(patsubst %.c,%.d,$(IPFRAG_SRC_LIB))
IPFRAG_DEP := $(patsubst %.c,%.d,$(IPFRAG_SRC))

CFLAGS_IPFRAG := -I$(DIRMISC) -I$(DIRIPHDR) -I$(DIRALLOC) -I$(DIRPACKET) -I$(DIRLINKEDLIST) -I$(DIRMYPCAP)
LIBS_IPFRAG := $(DIRALLOC)/liballoc.a $(DIRIPHDR)/libiphdr.a $(DIRMYPCAP)/libmypcap.a

MAKEFILES_IPFRAG := $(DIRIPFRAG)/module.mk

.PHONY: IPFRAG clean_IPFRAG distclean_IPFRAG unit_IPFRAG $(LCIPFRAG) clean_$(LCIPFRAG) distclean_$(LCIPFRAG) unit_$(LCIPFRAG)

$(LCIPFRAG): IPFRAG
clean_$(LCIPFRAG): clean_IPFRAG
distclean_$(LCIPFRAG): distclean_IPFRAG
unit_$(LCIPFRAG): unit_IPFRAG

IPFRAG: $(DIRIPFRAG)/libipfrag.a $(DIRIPFRAG)/ipfragtest $(DIRIPFRAG)/ipreasstest

unit_IPFRAG:
	@true

$(DIRIPFRAG)/libipfrag.a: $(IPFRAG_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_IPFRAG)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRIPFRAG)/ipfragtest: $(DIRIPFRAG)/ipfragtest.o $(DIRIPFRAG)/libipfrag.a $(LIBS_IPFRAG) $(MAKEFILES_COMMON) $(MAKEFILES_IPFRAG)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_IPFRAG)

$(DIRIPFRAG)/ipreasstest: $(DIRIPFRAG)/ipreasstest.o $(DIRIPFRAG)/libipfrag.a $(LIBS_IPFRAG) $(MAKEFILES_COMMON) $(MAKEFILES_IPFRAG)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_IPFRAG)

$(IPFRAG_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_IPFRAG)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_IPFRAG)

$(IPFRAG_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_IPFRAG)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_IPFRAG)

clean_IPFRAG:
	rm -f $(IPFRAG_OBJ) $(IPFRAG_DEP)

distclean_IPFRAG: clean_IPFRAG
	rm -f $(DIRIPFRAG)/libipfrag.a $(DIRIPFRAG)/ipfragtest $(DIRIPFRAG)/ipreasstest

-include $(DIRIPFRAG)/*.d
