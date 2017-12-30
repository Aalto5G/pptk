RBTREE_SRC_LIB := rbtree.c
RBTREE_SRC := $(RBTREE_SRC_LIB) rbtreetest.c

RBTREE_SRC_LIB := $(patsubst %,$(DIRRBTREE)/%,$(RBTREE_SRC_LIB))
RBTREE_SRC := $(patsubst %,$(DIRRBTREE)/%,$(RBTREE_SRC))

RBTREE_OBJ_LIB := $(patsubst %.c,%.o,$(RBTREE_SRC_LIB))
RBTREE_OBJ := $(patsubst %.c,%.o,$(RBTREE_SRC))

RBTREE_DEP_LIB := $(patsubst %.c,%.d,$(RBTREE_SRC_LIB))
RBTREE_DEP := $(patsubst %.c,%.d,$(RBTREE_SRC))

CFLAGS_RBTREE := -I$(DIRHASHLIST) -I$(DIRMISC)
LIBS_RBTREE := $(DIRMISC)/libmisc.a

MAKEFILES_RBTREE := $(DIRRBTREE)/module.mk

.PHONY: RBTREE clean_RBTREE distclean_RBTREE unit_RBTREE $(LCRBTREE) clean_$(LCRBTREE) distclean_$(LCRBTREE) unit_$(LCRBTREE)

$(LCRBTREE): RBTREE
clean_$(LCRBTREE): clean_RBTREE
distclean_$(LCRBTREE): distclean_RBTREE
unit_$(LCRBTREE): unit_RBTREE

RBTREE: $(DIRRBTREE)/librbtree.a $(DIRRBTREE)/rbtreetest

unit_RBTREE: $(DIRRBTREE)/rbtreetest
	$(DIRRBTREE)/rbtreetest

$(DIRRBTREE)/librbtree.a: $(RBTREE_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_RBTREE)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRRBTREE)/rbtreetest: $(DIRRBTREE)/rbtreetest.o $(DIRRBTREE)/librbtree.a $(LIBS_RBTREE) $(MAKEFILES_COMMON) $(MAKEFILES_RBTREE)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_RBTREE)

$(RBTREE_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_RBTREE)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_RBTREE)
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c $(CFLAGS_RBTREE)

$(RBTREE_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_RBTREE)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_RBTREE)

clean_RBTREE:
	rm -f $(RBTREE_OBJ) $(RBTREE_DEP)

distclean_RBTREE: clean_RBTREE
	rm -f $(DIRRBTREE)/librbtree.a $(DIRRBTREE)/rbtreetest

-include $(DIRRBTREE)/*.d
