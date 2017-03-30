HASHLIST_SRC_LIB :=
HASHLIST_SRC := $(HASHLIST_SRC_LIB)

HASHLIST_SRC_LIB := $(patsubst %,$(DIRHASHLIST)/%,$(HASHLIST_SRC_LIB))
HASHLIST_SRC := $(patsubst %,$(DIRHASHLIST)/%,$(HASHLIST_SRC))

HASHLIST_OBJ_LIB := $(patsubst %.c,%.o,$(HASHLIST_SRC_LIB))
HASHLIST_OBJ := $(patsubst %.c,%.o,$(HASHLIST_SRC))

HASHLIST_DEP_LIB := $(patsubst %.c,%.d,$(HASHLIST_SRC_LIB))
HASHLIST_DEP := $(patsubst %.c,%.d,$(HASHLIST_SRC))

CFLAGS_HASHLIST :=

MAKEFILES_HASHLIST := $(DIRHASHLIST)/module.mk

.PHONY: HASHLIST clean_HASHLIST distclean_HASHLIST unit_HASHLIST $(LCHASHLIST) clean_$(LCHASHLIST) distclean_$(LCHASHLIST) unit_$(LCHASHLIST)

$(LCHASHLIST): HASHLIST
clean_$(LCHASHLIST): clean_HASHLIST
distclean_$(LCHASHLIST): distclean_HASHLIST
unit_$(LCHASHLIST): unit_HASHLIST

HASHLIST: $(DIRHASHLIST)/libhashlist.a

unit_HASHLIST:
	@true

$(DIRHASHLIST)/libhashlist.a: $(HASHLIST_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_HASHLIST)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(HASHLIST_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_HASHLIST)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_HASHLIST)

$(HASHLIST_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_HASHLIST)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_HASHLIST)

clean_HASHLIST:
	rm -f $(HASHLIST_OBJ) $(HASHLIST_DEP)

distclean_HASHLIST: clean_HASHLIST
	rm -f $(DIRHASHLIST)/libhashlist.a

-include $(DIRHASHLIST)/*.d
