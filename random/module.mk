RANDOM_SRC_LIB := random_mt.c
RANDOM_SRC := $(RANDOM_SRC_LIB) random_mt_test.c

RANDOM_SRC_LIB := $(patsubst %,$(DIRRANDOM)/%,$(RANDOM_SRC_LIB))
RANDOM_SRC := $(patsubst %,$(DIRRANDOM)/%,$(RANDOM_SRC))

RANDOM_OBJ_LIB := $(patsubst %.c,%.o,$(RANDOM_SRC_LIB))
RANDOM_OBJ := $(patsubst %.c,%.o,$(RANDOM_SRC))

RANDOM_DEP_LIB := $(patsubst %.c,%.d,$(RANDOM_SRC_LIB))
RANDOM_DEP := $(patsubst %.c,%.d,$(RANDOM_SRC))

CFLAGS_RANDOM := -I$(DIRHASHLIST) -I$(DIRMISC) -I$(DIRQUEUE) -I$(DIRLINKEDLIST)

LIBS_RANDOM :=

MAKEFILES_RANDOM := $(DIRRANDOM)/module.mk

.PHONY: RANDOM clean_RANDOM distclean_RANDOM unit_RANDOM $(LCRANDOM) clean_$(LCRANDOM) distclean_$(LCRANDOM) unit_$(LCRANDOM)

$(LCRANDOM): RANDOM
clean_$(LCRANDOM): clean_RANDOM
distclean_$(LCRANDOM): distclean_RANDOM
unit_$(LCRANDOM): unit_RANDOM

RANDOM: $(DIRRANDOM)/librandom.a $(DIRRANDOM)/random_mt_test

unit_RANDOM: $(DIRRANDOM)/random_mt_test
	$(DIRRANDOM)/random_mt_test

$(DIRRANDOM)/librandom.a: $(RANDOM_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_RANDOM)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRRANDOM)/random_mt_test: $(DIRRANDOM)/random_mt_test.o $(DIRRANDOM)/librandom.a $(LIBS_RANDOM) $(MAKEFILES_COMMON) $(MAKEFILES_RANDOM)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_RANDOM) -lpthread

$(RANDOM_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_RANDOM)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_RANDOM)

$(RANDOM_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_RANDOM)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_RANDOM)

clean_RANDOM:
	rm -f $(RANDOM_OBJ) $(RANDOM_DEP)

distclean_RANDOM: clean_RANDOM
	rm -f $(DIRRANDOM)/librandom.a $(DIRRANDOM)/random_mt_test

-include $(DIRRANDOM)/*.d
