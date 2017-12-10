TIMERRB_SRC_LIB := timerrb.c
TIMERRB_SRC := $(TIMERRB_SRC_LIB) timerrbtest.c

TIMERRB_SRC_LIB := $(patsubst %,$(DIRTIMERRB)/%,$(TIMERRB_SRC_LIB))
TIMERRB_SRC := $(patsubst %,$(DIRTIMERRB)/%,$(TIMERRB_SRC))

TIMERRB_OBJ_LIB := $(patsubst %.c,%.o,$(TIMERRB_SRC_LIB))
TIMERRB_OBJ := $(patsubst %.c,%.o,$(TIMERRB_SRC))

TIMERRB_DEP_LIB := $(patsubst %.c,%.d,$(TIMERRB_SRC_LIB))
TIMERRB_DEP := $(patsubst %.c,%.d,$(TIMERRB_SRC))

CFLAGS_TIMERRB := -I$(DIRHASHLIST) -I$(DIRMISC) -I$(DIRRBTREE)
LIBS_TIMERRB := $(DIRMISC)/libmisc.a $(DIRRBTREE)/librbtree.a

MAKEFILES_TIMERRB := $(DIRTIMERRB)/module.mk

.PHONY: TIMERRB clean_TIMERRB distclean_TIMERRB unit_TIMERRB $(LCTIMERRB) clean_$(LCTIMERRB) distclean_$(LCTIMERRB) unit_$(LCTIMERRB)

$(LCTIMERRB): TIMERRB
clean_$(LCTIMERRB): clean_TIMERRB
distclean_$(LCTIMERRB): distclean_TIMERRB
unit_$(LCTIMERRB): unit_TIMERRB

TIMERRB: $(DIRTIMERRB)/libtimerrb.a $(DIRTIMERRB)/timerrbtest

unit_TIMERRB: $(DIRTIMERRB)/timerrbtest
	$(DIRTIMERRB)/timerrbtest

$(DIRTIMERRB)/libtimerrb.a: $(TIMERRB_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_TIMERRB)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRTIMERRB)/timerrbtest: $(DIRTIMERRB)/timerrbtest.o $(DIRTIMERRB)/libtimerrb.a $(LIBS_TIMERRB) $(MAKEFILES_COMMON) $(MAKEFILES_TIMERRB)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_TIMERRB)

$(TIMERRB_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_TIMERRB)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_TIMERRB)

$(TIMERRB_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_TIMERRB)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_TIMERRB)

clean_TIMERRB:
	rm -f $(TIMERRB_OBJ) $(TIMERRB_DEP)

distclean_TIMERRB: clean_TIMERRB
	rm -f $(DIRTIMERRB)/libtimerrb.a $(DIRTIMERRB)/timerrbtest

-include $(DIRTIMERRB)/*.d
