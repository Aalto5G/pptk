IPHASH_SRC_LIB := 
IPHASH_SRC := $(IPHASH_SRC_LIB) iphashtest.c iphashtest2.c

IPHASH_SRC_LIB := $(patsubst %,$(DIRIPHASH)/%,$(IPHASH_SRC_LIB))
IPHASH_SRC := $(patsubst %,$(DIRIPHASH)/%,$(IPHASH_SRC))

IPHASH_OBJ_LIB := $(patsubst %.c,%.o,$(IPHASH_SRC_LIB))
IPHASH_OBJ := $(patsubst %.c,%.o,$(IPHASH_SRC))

IPHASH_DEP_LIB := $(patsubst %.c,%.d,$(IPHASH_SRC_LIB))
IPHASH_DEP := $(patsubst %.c,%.d,$(IPHASH_SRC))

CFLAGS_IPHASH := -I$(DIRHASHLIST) -I$(DIRMISC) -I$(DIRHASHTABLE) -I$(DIRTIMERLINKHEAP)
LIBS_IPHASH := $(DIRTIMERLINKHEAP)/libtimerlinkheap.a

MAKEFILES_IPHASH := $(DIRIPHASH)/module.mk

.PHONY: IPHASH clean_IPHASH distclean_IPHASH unit_IPHASH $(LCIPHASH) clean_$(LCIPHASH) distclean_$(LCIPHASH) unit_$(LCIPHASH)

$(LCIPHASH): IPHASH
clean_$(LCIPHASH): clean_IPHASH
distclean_$(LCIPHASH): distclean_IPHASH
unit_$(LCIPHASH): unit_IPHASH

IPHASH: $(DIRIPHASH)/libiphash.a $(DIRIPHASH)/iphashtest $(DIRIPHASH)/iphashtest2

unit_IPHASH:
	@true

$(DIRIPHASH)/libiphash.a: $(IPHASH_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_IPHASH)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRIPHASH)/iphashtest: $(DIRIPHASH)/iphashtest.o $(DIRIPHASH)/libiphash.a $(LIBS_IPHASH) $(MAKEFILES_COMMON) $(MAKEFILES_IPHASH)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_IPHASH)

$(DIRIPHASH)/iphashtest2: $(DIRIPHASH)/iphashtest2.o $(DIRIPHASH)/libiphash.a $(LIBS_IPHASH) $(MAKEFILES_COMMON) $(MAKEFILES_IPHASH)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_IPHASH)

$(IPHASH_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_IPHASH)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_IPHASH)

$(IPHASH_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_IPHASH)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_IPHASH)

clean_IPHASH:
	rm -f $(IPHASH_OBJ) $(IPHASH_DEP)

distclean_IPHASH: clean_IPHASH
	rm -f $(DIRIPHASH)/libiphash.a $(DIRIPHASH)/iphashtest

-include $(DIRIPHASH)/*.d
