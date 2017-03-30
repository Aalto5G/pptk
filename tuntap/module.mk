TUNTAP_SRC_LIB := tuntap.c
TUNTAP_SRC := $(TUNTAP_SRC_LIB) tapsilent.c

TUNTAP_SRC_LIB := $(patsubst %,$(DIRTUNTAP)/%,$(TUNTAP_SRC_LIB))
TUNTAP_SRC := $(patsubst %,$(DIRTUNTAP)/%,$(TUNTAP_SRC))

TUNTAP_OBJ_LIB := $(patsubst %.c,%.o,$(TUNTAP_SRC_LIB))
TUNTAP_OBJ := $(patsubst %.c,%.o,$(TUNTAP_SRC))

TUNTAP_DEP_LIB := $(patsubst %.c,%.d,$(TUNTAP_SRC_LIB))
TUNTAP_DEP := $(patsubst %.c,%.d,$(TUNTAP_SRC))

CFLAGS_TUNTAP :=

MAKEFILES_TUNTAP := $(DIRTUNTAP)/module.mk

.PHONY: TUNTAP clean_TUNTAP distclean_TUNTAP unit_TUNTAP $(LCTUNTAP) clean_$(LCTUNTAP) distclean_$(LCTUNTAP) unit_$(LCTUNTAP)

$(LCTUNTAP): TUNTAP
clean_$(LCTUNTAP): clean_TUNTAP
distclean_$(LCTUNTAP): distclean_TUNTAP
unit_$(LCTUNTAP): unit_TUNTAP

TUNTAP: $(DIRTUNTAP)/libtuntap.a $(DIRTUNTAP)/tapsilent

unit_TUNTAP:
	@true

$(DIRTUNTAP)/libtuntap.a: $(TUNTAP_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_TUNTAP)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRTUNTAP)/tapsilent: $(DIRTUNTAP)/tapsilent.o $(DIRTUNTAP)/libtuntap.a $(MAKEFILES_COMMON) $(MAKEFILES_TUNTAP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_TUNTAP)

$(TUNTAP_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_TUNTAP)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_TUNTAP)

$(TUNTAP_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_TUNTAP)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_TUNTAP)

clean_TUNTAP:
	rm -f $(TUNTAP_OBJ) $(TUNTAP_DEP)

distclean_TUNTAP: clean_TUNTAP
	rm -f $(DIRTUNTAP)/libtuntap.a $(DIRTUNTAP)/tapsilent

-include $(DIRTUNTAP)/*.d
