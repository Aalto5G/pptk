ALLOC_SRC_LIB := 
ALLOC_SRC := $(ALLOC_SRC_LIB)

ALLOC_SRC_LIB := $(patsubst %,$(DIRALLOC)/%,$(ALLOC_SRC_LIB))
ALLOC_SRC := $(patsubst %,$(DIRALLOC)/%,$(ALLOC_SRC))

ALLOC_OBJ_LIB := $(patsubst %.c,%.o,$(ALLOC_SRC_LIB))
ALLOC_OBJ := $(patsubst %.c,%.o,$(ALLOC_SRC))

ALLOC_DEP_LIB := $(patsubst %.c,%.d,$(ALLOC_SRC_LIB))
ALLOC_DEP := $(patsubst %.c,%.d,$(ALLOC_SRC))

CFLAGS_ALLOC := -I$(DIRHASHLIST) -I$(DIRMISC)

LIBS_ALLOC :=

MAKEFILES_ALLOC := $(DIRALLOC)/module.mk

.PHONY: ALLOC clean_ALLOC distclean_ALLOC unit_ALLOC $(LCALLOC) clean_$(LCALLOC) distclean_$(LCALLOC) unit_$(LCALLOC)

$(LCALLOC): ALLOC
clean_$(LCALLOC): clean_ALLOC
distclean_$(LCALLOC): distclean_ALLOC

ALLOC: $(DIRALLOC)/liballoc.a

unit_ALLOC:
	@true

$(DIRALLOC)/liballoc.a: $(ALLOC_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(ALLOC_OBJ): %.o: %.c $(ALLOC_DEP) $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_ALLOC)

$(ALLOC_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_ALLOC)

clean_ALLOC:
	rm -f $(ALLOC_OBJ) $(ALLOC_DEP)

distclean_ALLOC: clean_ALLOC
	rm -f $(DIRALLOC)/liballoc.a

-include $(DIRALLOC)/*.d
