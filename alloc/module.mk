ALLOC_SRC_LIB := asalloc.c rballoc.c llalloc.c directalloc.c
ALLOC_SRC := $(ALLOC_SRC_LIB) asperfmt.c asperfst.c mallocperfmt.c rbperfmt.c rbperfst.c directallocperfmt.c llperfmt.c llperfst.c allociftest.c

ALLOC_SRC_LIB := $(patsubst %,$(DIRALLOC)/%,$(ALLOC_SRC_LIB))
ALLOC_SRC := $(patsubst %,$(DIRALLOC)/%,$(ALLOC_SRC))

ALLOC_OBJ_LIB := $(patsubst %.c,%.o,$(ALLOC_SRC_LIB))
ALLOC_OBJ := $(patsubst %.c,%.o,$(ALLOC_SRC))

ALLOC_DEP_LIB := $(patsubst %.c,%.d,$(ALLOC_SRC_LIB))
ALLOC_DEP := $(patsubst %.c,%.d,$(ALLOC_SRC))

CFLAGS_ALLOC := -I$(DIRHASHLIST) -I$(DIRMISC) -I$(DIRQUEUE) -I$(DIRLINKEDLIST)

LIBS_ALLOC := $(DIRQUEUE)/libqueue.a

MAKEFILES_ALLOC := $(DIRALLOC)/module.mk

.PHONY: ALLOC clean_ALLOC distclean_ALLOC unit_ALLOC $(LCALLOC) clean_$(LCALLOC) distclean_$(LCALLOC) unit_$(LCALLOC)

$(LCALLOC): ALLOC
clean_$(LCALLOC): clean_ALLOC
distclean_$(LCALLOC): distclean_ALLOC
unit_$(LCALLOC): unit_ALLOC

ALLOC: $(DIRALLOC)/liballoc.a $(DIRALLOC)/asperfmt $(DIRALLOC)/asperfst $(DIRALLOC)/mallocperfmt $(DIRALLOC)/rbperfmt $(DIRALLOC)/rbperfst $(DIRALLOC)/directallocperfmt $(DIRALLOC)/llperfmt $(DIRALLOC)/llperfst $(DIRALLOC)/allociftest

unit_ALLOC: $(DIRALLOC)/llperfst $(DIRALLOC)/allociftest
	$(DIRALLOC)/asperfmt
	$(DIRALLOC)/asperfst
	$(DIRALLOC)/rbperfmt
	$(DIRALLOC)/rbperfst
	$(DIRALLOC)/mallocperfmt
	$(DIRALLOC)/directallocperfmt
	$(DIRALLOC)/llperfmt
	$(DIRALLOC)/llperfst
	$(DIRALLOC)/allociftest

$(DIRALLOC)/liballoc.a: $(ALLOC_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRALLOC)/allociftest: $(DIRALLOC)/allociftest.o $(DIRALLOC)/liballoc.a $(LIBS_ALLOC) $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_ALLOC) -lpthread

$(DIRALLOC)/asperfmt: $(DIRALLOC)/asperfmt.o $(DIRALLOC)/liballoc.a $(LIBS_ALLOC) $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_ALLOC) -lpthread

$(DIRALLOC)/asperfst: $(DIRALLOC)/asperfst.o $(DIRALLOC)/liballoc.a $(LIBS_ALLOC) $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_ALLOC) -lpthread

$(DIRALLOC)/rbperfmt: $(DIRALLOC)/rbperfmt.o $(DIRALLOC)/liballoc.a $(LIBS_ALLOC) $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_ALLOC) -lpthread

$(DIRALLOC)/rbperfst: $(DIRALLOC)/rbperfst.o $(DIRALLOC)/liballoc.a $(LIBS_ALLOC) $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_ALLOC) -lpthread

$(DIRALLOC)/llperfmt: $(DIRALLOC)/llperfmt.o $(DIRALLOC)/liballoc.a $(LIBS_ALLOC) $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_ALLOC) -lpthread

$(DIRALLOC)/llperfst: $(DIRALLOC)/llperfst.o $(DIRALLOC)/liballoc.a $(LIBS_ALLOC) $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_ALLOC) -lpthread

$(DIRALLOC)/mallocperfmt: $(DIRALLOC)/mallocperfmt.o $(DIRALLOC)/liballoc.a $(LIBS_ALLOC) $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_ALLOC) -lpthread

$(DIRALLOC)/directallocperfmt: $(DIRALLOC)/directallocperfmt.o $(DIRALLOC)/liballoc.a $(LIBS_ALLOC) $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_ALLOC) -lpthread

$(ALLOC_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_ALLOC)
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c $(CFLAGS_ALLOC)

$(ALLOC_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_ALLOC)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_ALLOC)

clean_ALLOC:
	rm -f $(ALLOC_OBJ) $(ALLOC_DEP)

distclean_ALLOC: clean_ALLOC
	rm -f $(DIRALLOC)/liballoc.a $(DIRALLOC)/asperfmt $(DIRALLOC)/asperfst $(DIRALLOC)/mallocperfmt $(DIRALLOC)/rbperfmt $(DIRALLOC)/rbperfst $(DIRALLOC)/directallocperfmt $(DIRALLOC)/llperfmt $(DIRALLOC)/llperfst $(DIRALLOC)/allociftest

-include $(DIRALLOC)/*.d
