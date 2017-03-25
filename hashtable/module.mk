HASHTABLE_SRC_LIB := 
HASHTABLE_SRC := $(HASHTABLE_SRC_LIB) hashtest.c

HASHTABLE_SRC_LIB := $(patsubst %,$(DIRHASHTABLE)/%,$(HASHTABLE_SRC_LIB))
HASHTABLE_SRC := $(patsubst %,$(DIRHASHTABLE)/%,$(HASHTABLE_SRC))

HASHTABLE_OBJ_LIB := $(patsubst %.c,%.o,$(HASHTABLE_SRC_LIB))
HASHTABLE_OBJ := $(patsubst %.c,%.o,$(HASHTABLE_SRC))

HASHTABLE_DEP_LIB := $(patsubst %.c,%.d,$(HASHTABLE_SRC_LIB))
HASHTABLE_DEP := $(patsubst %.c,%.d,$(HASHTABLE_SRC))

CFLAGS_HASHTABLE := -I$(DIRHASHLIST) -I$(DIRMISC)

.PHONY: HASHTABLE clean_HASHTABLE distclean_HASHTABLE unit_HASHTABLE $(LCHASHTABLE) clean_$(LCHASHTABLE) distclean_$(LCHASHTABLE) unit_$(LCHASHTABLE)

$(LCHASHTABLE): HASHTABLE
clean_$(LCHASHTABLE): clean_HASHTABLE
distclean_$(LCHASHTABLE): distclean_HASHTABLE

HASHTABLE: $(DIRHASHTABLE)/libhashtable.a $(DIRHASHTABLE)/hashtest

unit_HASHTABLE: $(DIRHASHTABLE)/hashtest
	$(DIRHASHTABLE)/hashtest

$(DIRHASHTABLE)/libhashtable.a: $(HASHTABLE_OBJ_LIB)
	rm -f $@
	ar rvs $@ $^

$(DIRHASHTABLE)/containeroftest: $(DIRHASHTABLE)/containeroftest.o $(DIRHASHTABLE)/libhashtable.a
	$(CC) $(CFLAGS) -o $@ $^ $(CFLAGS_HASHTABLE)

$(DIRHASHTABLE)/murmurtest: $(DIRHASHTABLE)/murmurtest.o $(DIRHASHTABLE)/libhashtable.a
	$(CC) $(CFLAGS) -o $@ $^ $(CFLAGS_HASHTABLE)

$(DIRHASHTABLE)/hashtest: $(DIRHASHTABLE)/hashtest.o $(DIRHASHTABLE)/libhashtable.a
	$(CC) $(CFLAGS) -o $@ $^ $(CFLAGS_HASHTABLE)

$(HASHTABLE_OBJ): %.o: %.c $(HASHTABLE_DEP)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_HASHTABLE)

$(HASHTABLE_DEP): %.d: %.c
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_HASHTABLE)

clean_HASHTABLE:
	rm -f $(HASHTABLE_OBJ)

distclean_HASHTABLE: clean_HASHTABLE
	rm -f $(DIRHASHTABLE)/libhashtable.a $(DIRHASHTABLE)/containeroftest

-include $(HASHTABLE_DEP)
