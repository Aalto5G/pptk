ALLOC_SRC_LIB := llalloc.c
ALLOC_SRC := $(ALLOC_SRC_LIB) llperfst.c

ALLOC_SRC_LIB := $(patsubst %,$(DIRALLOC)/%,$(ALLOC_SRC_LIB))
ALLOC_SRC := $(patsubst %,$(DIRALLOC)/%,$(ALLOC_SRC))

ALLOC_OBJ_LIB := $(patsubst %.c,%.o,$(ALLOC_SRC_LIB))
ALLOC_OBJ := $(patsubst %.c,%.o,$(ALLOC_SRC))

ALLOC_DEP_LIB := $(patsubst %.c,%.d,$(ALLOC_SRC_LIB))
ALLOC_DEP := $(patsubst %.c,%.d,$(ALLOC_SRC))

CFLAGS_ALLOC := -I$(DIRHASHLIST) -I$(DIRMISC) -I$(DIRLINKEDLIST)

LIBS_ALLOC :=

MAKEFILES_ALLOC := $(DIRALLOC)/module.mk

.PHONY: ALLOC clean_ALLOC distclean_ALLOC unit_ALLOC $(LCALLOC) clean_$(LCALLOC) distclean_$(LCALLOC) unit_$(LCALLOC)

$(LCALLOC): ALLOC
clean_$(LCALLOC): clean_ALLOC
distclean_$(LCALLOC): distclean_ALLOC
unit_$(LCALLOC): unit_ALLOC

ALLOC: $(DIRALLOC)/liballoc.a $(DIRALLOC)/llperfst

unit_ALLOC: $(DIRALLOC)/llperfst
	$(DIRALLOC)/llperfst

$(DIRALLOC)/liballoc.a: $(ALLOC_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRALLOC)/llperfst: $(DIRALLOC)/llperfst.o $(DIRALLOC)/liballoc.a $(LIBS_ALLOC) $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_ALLOC) -lpthread

$(ALLOC_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_ALLOC)

$(ALLOC_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_ALLOC)

clean_ALLOC:
	rm -f $(ALLOC_OBJ) $(ALLOC_DEP)

distclean_ALLOC: clean_ALLOC
	rm -f $(DIRALLOC)/liballoc.a $(DIRALLOC)/llperfst

-include $(DIRALLOC)/*.d
