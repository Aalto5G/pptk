DYNARR_SRC_LIB := dynarr.c
DYNARR_SRC := $(DYNARR_SRC_LIB) dynarrtest.c

DYNARR_SRC_LIB := $(patsubst %,$(DIRDYNARR)/%,$(DYNARR_SRC_LIB))
DYNARR_SRC := $(patsubst %,$(DIRDYNARR)/%,$(DYNARR_SRC))

DYNARR_OBJ_LIB := $(patsubst %.c,%.o,$(DYNARR_SRC_LIB))
DYNARR_OBJ := $(patsubst %.c,%.o,$(DYNARR_SRC))

DYNARR_DEP_LIB := $(patsubst %.c,%.d,$(DYNARR_SRC_LIB))
DYNARR_DEP := $(patsubst %.c,%.d,$(DYNARR_SRC))

CFLAGS_DYNARR :=

MAKEFILES_DYNARR := $(DIRDYNARR)/module.mk

.PHONY: DYNARR clean_DYNARR distclean_DYNARR unit_DYNARR $(LCDYNARR) clean_$(LCDYNARR) distclean_$(LCDYNARR) unit_$(LCDYNARR)

$(LCDYNARR): DYNARR
clean_$(LCDYNARR): clean_DYNARR
distclean_$(LCDYNARR): distclean_DYNARR

DYNARR: $(DIRDYNARR)/libdynarr.a $(DIRDYNARR)/dynarrtest

unit_DYNARR: $(DIRDYNARR)/dynarrtest
	$(DIRDYNARR)/dynarrtest

$(DIRDYNARR)/libdynarr.a: $(DYNARR_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_DYNARR)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRDYNARR)/dynarrtest: $(DIRDYNARR)/dynarrtest.o $(DIRDYNARR)/libdynarr.a $(MAKEFILES_COMMON) $(MAKEFILES_DYNARR)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_DYNARR)

$(DYNARR_OBJ): %.o: %.c $(DYNARR_DEP) $(MAKEFILES_COMMON) $(MAKEFILES_DYNARR)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_DYNARR)

$(DYNARR_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_DYNARR)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_DYNARR)

clean_DYNARR:
	rm -f $(DYNARR_OBJ) $(DYNARR_DEP)

distclean_DYNARR: clean_DYNARR
	rm -f $(DIRDYNARR)/libdynarr.a $(DIRDYNARR)/dynarrtest

-include $(DIRDYNARR)/*.d
