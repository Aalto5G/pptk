AVLTREE_SRC_LIB := avltree.c
AVLTREE_SRC := $(AVLTREE_SRC_LIB) avltreetest.c

AVLTREE_SRC_LIB := $(patsubst %,$(DIRAVLTREE)/%,$(AVLTREE_SRC_LIB))
AVLTREE_SRC := $(patsubst %,$(DIRAVLTREE)/%,$(AVLTREE_SRC))

AVLTREE_OBJ_LIB := $(patsubst %.c,%.o,$(AVLTREE_SRC_LIB))
AVLTREE_OBJ := $(patsubst %.c,%.o,$(AVLTREE_SRC))

AVLTREE_DEP_LIB := $(patsubst %.c,%.d,$(AVLTREE_SRC_LIB))
AVLTREE_DEP := $(patsubst %.c,%.d,$(AVLTREE_SRC))

CFLAGS_AVLTREE := -I$(DIRHASHLIST) -I$(DIRMISC)
LIBS_AVLTREE := $(DIRMISC)/libmisc.a

MAKEFILES_AVLTREE := $(DIRAVLTREE)/module.mk

.PHONY: AVLTREE clean_AVLTREE distclean_AVLTREE unit_AVLTREE $(LCAVLTREE) clean_$(LCAVLTREE) distclean_$(LCAVLTREE) unit_$(LCAVLTREE)

$(LCAVLTREE): AVLTREE
clean_$(LCAVLTREE): clean_AVLTREE
distclean_$(LCAVLTREE): distclean_AVLTREE
unit_$(LCAVLTREE): unit_AVLTREE

AVLTREE: $(DIRAVLTREE)/libavltree.a $(DIRAVLTREE)/avltreetest

unit_AVLTREE: $(DIRAVLTREE)/avltreetest
	$(DIRAVLTREE)/avltreetest

$(DIRAVLTREE)/libavltree.a: $(AVLTREE_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_AVLTREE)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRAVLTREE)/avltreetest: $(DIRAVLTREE)/avltreetest.o $(DIRAVLTREE)/libavltree.a $(LIBS_AVLTREE) $(MAKEFILES_COMMON) $(MAKEFILES_AVLTREE)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_AVLTREE)

$(AVLTREE_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_AVLTREE)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_AVLTREE)
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c $(CFLAGS_AVLTREE)

$(AVLTREE_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_AVLTREE)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_AVLTREE)

clean_AVLTREE:
	rm -f $(AVLTREE_OBJ) $(AVLTREE_DEP)

distclean_AVLTREE: clean_AVLTREE
	rm -f $(DIRAVLTREE)/libavltree.a $(DIRAVLTREE)/avltreetest

-include $(DIRAVLTREE)/*.d
