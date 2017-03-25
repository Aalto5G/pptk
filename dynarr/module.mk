DYNARR_SRC_LIB := dynarr.c
DYNARR_SRC := $(DYNARR_SRC_LIB) dynarrtest.c

DYNARR_SRC_LIB := $(patsubst %,$(DIRDYNARR)/%,$(DYNARR_SRC_LIB))
DYNARR_SRC := $(patsubst %,$(DIRDYNARR)/%,$(DYNARR_SRC))

DYNARR_OBJ_LIB := $(patsubst %.c,%.o,$(DYNARR_SRC_LIB))
DYNARR_OBJ := $(patsubst %.c,%.o,$(DYNARR_SRC))

.PHONY: DYNARR clean_DYNARR distclean_DYNARR

DYNARR: $(DIRDYNARR)/libdynarr.a $(DIRDYNARR)/dynarrtest

$(DIRDYNARR)/libdynarr.a: $(DYNARR_OBJ_LIB)
	rm -f $@
	ar rvs $@ $^

$(DIRDYNARR)/dynarrtest: $(DIRDYNARR)/dynarrtest.o $(DIRDYNARR)/libdynarr.a
	$(CC) $(CFLAGS) -o $@ $^

$(DYNARR_OBJ): %.o: %.c
	$(CC) $(CFLAGS) -c -o $*.o $*.c

clean_DYNARR:
	rm -f $(DYNARR_OBJ)

distclean_DYNARR: clean_DYNARR
	rm -f $(DIRDYNARR)/libdynarr.a $(DIRDYNARR)/dynarrtest
