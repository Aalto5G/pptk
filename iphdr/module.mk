IPHDR_SRC_LIB := 
IPHDR_SRC := $(IPHDR_SRC_LIB) iphdrtest.c

IPHDR_SRC_LIB := $(patsubst %,$(DIRIPHDR)/%,$(IPHDR_SRC_LIB))
IPHDR_SRC := $(patsubst %,$(DIRIPHDR)/%,$(IPHDR_SRC))

IPHDR_OBJ_LIB := $(patsubst %.c,%.o,$(IPHDR_SRC_LIB))
IPHDR_OBJ := $(patsubst %.c,%.o,$(IPHDR_SRC))

IPHDR_DEP_LIB := $(patsubst %.c,%.d,$(IPHDR_SRC_LIB))
IPHDR_DEP := $(patsubst %.c,%.d,$(IPHDR_SRC))

CFLAGS_IPHDR := -I$(DIRMISC)

MAKEFILES_IPHDR := $(DIRIPHDR)/module.mk

.PHONY: IPHDR clean_IPHDR distclean_IPHDR unit_IPHDR $(LCIPHDR) clean_$(LCIPHDR) distclean_$(LCIPHDR) unit_$(LCIPHDR)

$(LCIPHDR): IPHDR
clean_$(LCIPHDR): clean_IPHDR
distclean_$(LCIPHDR): distclean_IPHDR

IPHDR: $(DIRIPHDR)/libiphdr.a $(DIRIPHDR)/iphdrtest

unit_IPHDR: $(DIRIPHDR)/iphdrtest
	$(DIRIPHDR)/iphdrtest

$(DIRIPHDR)/libiphdr.a: $(IPHDR_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_IPHDR)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRIPHDR)/iphdrtest: $(DIRIPHDR)/iphdrtest.o $(DIRIPHDR)/libiphdr.a $(MAKEFILES_COMMON) $(MAKEFILES_IPHDR)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_IPHDR)

$(IPHDR_OBJ): %.o: %.c $(IPHDR_DEP) $(MAKEFILES_COMMON) $(MAKEFILES_IPHDR)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_IPHDR)

$(IPHDR_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_IPHDR)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_IPHDR)

clean_IPHDR:
	rm -f $(IPHDR_OBJ) $(IPHDR_DEP)

distclean_IPHDR: clean_IPHDR
	rm -f $(DIRIPHDR)/libiphdr.a $(DIRIPHDR)/containeroftest

-include $(IPHDR_DEP)
