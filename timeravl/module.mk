TIMERAVL_SRC_LIB := timeravl.c
TIMERAVL_SRC := $(TIMERAVL_SRC_LIB) timeravltest.c

TIMERAVL_SRC_LIB := $(patsubst %,$(DIRTIMERAVL)/%,$(TIMERAVL_SRC_LIB))
TIMERAVL_SRC := $(patsubst %,$(DIRTIMERAVL)/%,$(TIMERAVL_SRC))

TIMERAVL_OBJ_LIB := $(patsubst %.c,%.o,$(TIMERAVL_SRC_LIB))
TIMERAVL_OBJ := $(patsubst %.c,%.o,$(TIMERAVL_SRC))

TIMERAVL_DEP_LIB := $(patsubst %.c,%.d,$(TIMERAVL_SRC_LIB))
TIMERAVL_DEP := $(patsubst %.c,%.d,$(TIMERAVL_SRC))

CFLAGS_TIMERAVL := -I$(DIRHASHLIST) -I$(DIRMISC) -I$(DIRAVLTREE)
LIBS_TIMERAVL := $(DIRMISC)/libmisc.a $(DIRAVLTREE)/libavltree.a

MAKEFILES_TIMERAVL := $(DIRTIMERAVL)/module.mk

.PHONY: TIMERAVL clean_TIMERAVL distclean_TIMERAVL unit_TIMERAVL $(LCTIMERAVL) clean_$(LCTIMERAVL) distclean_$(LCTIMERAVL) unit_$(LCTIMERAVL)

$(LCTIMERAVL): TIMERAVL
clean_$(LCTIMERAVL): clean_TIMERAVL
distclean_$(LCTIMERAVL): distclean_TIMERAVL
unit_$(LCTIMERAVL): unit_TIMERAVL

TIMERAVL: $(DIRTIMERAVL)/libtimeravl.a $(DIRTIMERAVL)/timeravltest

unit_TIMERAVL:
	@true

$(DIRTIMERAVL)/libtimeravl.a: $(TIMERAVL_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_TIMERAVL)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRTIMERAVL)/timeravltest: $(DIRTIMERAVL)/timeravltest.o $(DIRTIMERAVL)/libtimeravl.a $(LIBS_TIMERAVL) $(MAKEFILES_COMMON) $(MAKEFILES_TIMERAVL)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_TIMERAVL)

$(TIMERAVL_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_TIMERAVL)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_TIMERAVL)
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c $(CFLAGS_TIMERAVL)

$(TIMERAVL_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_TIMERAVL)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_TIMERAVL)

clean_TIMERAVL:
	rm -f $(TIMERAVL_OBJ) $(TIMERAVL_DEP)

distclean_TIMERAVL: clean_TIMERAVL
	rm -f $(DIRTIMERAVL)/libtimeravl.a $(DIRTIMERAVL)/timeravltest

-include $(DIRTIMERAVL)/*.d
