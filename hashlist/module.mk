HASHLIST_SRC_LIB :=
HASHLIST_SRC := $(HASHLIST_SRC_LIB)

HASHLIST_SRC_LIB := $(patsubst %,$(DIRHASHLIST)/%,$(HASHLIST_SRC_LIB))
HASHLIST_SRC := $(patsubst %,$(DIRHASHLIST)/%,$(HASHLIST_SRC))

HASHLIST_OBJ_LIB := $(patsubst %.c,%.o,$(HASHLIST_SRC_LIB))
HASHLIST_OBJ := $(patsubst %.c,%.o,$(HASHLIST_SRC))

HASHLIST_DEP_LIB := $(patsubst %.c,%.d,$(HASHLIST_SRC_LIB))
HASHLIST_DEP := $(patsubst %.c,%.d,$(HASHLIST_SRC))

.PHONY: HASHLIST clean_HASHLIST distclean_HASHLIST unit_HASHLIST $(LCHASHLIST) clean_$(LCHASHLIST) distclean_$(LCHASHLIST) unit_$(LCHASHLIST)

$(LCHASHLIST): HASHLIST
clean_$(LCHASHLIST): clean_HASHLIST
distclean_$(LCHASHLIST): distclean_HASHLIST

HASHLIST: $(DIRHASHLIST)/libhashlist.a

unit_HASHLIST:
	@true

$(DIRHASHLIST)/libhashlist.a: $(HASHLIST_OBJ_LIB)
	rm -f $@
	ar rvs $@ $^

$(HASHLIST_OBJ): %.o: %.c $(HASHLIST_DEP)
	$(CC) $(CFLAGS) -c -o $*.o $*.c

$(HASHLIST_DEP): %.d: %.c
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c

clean_HASHLIST:
	rm -f $(HASHLIST_OBJ)

distclean_HASHLIST: clean_HASHLIST
	rm -f $(DIRHASHLIST)/libhashlist.a $(DIRHASHLIST)/hashlisttest

-include $(HASHLIST_DEP)
