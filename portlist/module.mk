PORTLIST_SRC_LIB :=
PORTLIST_SRC := $(PORTLIST_SRC_LIB) portlisttest.c

PORTLIST_SRC_LIB := $(patsubst %,$(DIRPORTLIST)/%,$(PORTLIST_SRC_LIB))
PORTLIST_SRC := $(patsubst %,$(DIRPORTLIST)/%,$(PORTLIST_SRC))

PORTLIST_OBJ_LIB := $(patsubst %.c,%.o,$(PORTLIST_SRC_LIB))
PORTLIST_OBJ := $(patsubst %.c,%.o,$(PORTLIST_SRC))

PORTLIST_DEP_LIB := $(patsubst %.c,%.d,$(PORTLIST_SRC_LIB))
PORTLIST_DEP := $(patsubst %.c,%.d,$(PORTLIST_SRC))

CFLAGS_PORTLIST := -I$(DIRLINKEDLIST) -I$(DIRMISC)

MAKEFILES_PORTLIST := $(DIRPORTLIST)/module.mk

.PHONY: PORTLIST clean_PORTLIST distclean_PORTLIST unit_PORTLIST $(LCPORTLIST) clean_$(LCPORTLIST) distclean_$(LCPORTLIST) unit_$(LCPORTLIST)

$(LCPORTLIST): PORTLIST
clean_$(LCPORTLIST): clean_PORTLIST
distclean_$(LCPORTLIST): distclean_PORTLIST
unit_$(LCPORTLIST): unit_PORTLIST

PORTLIST: $(DIRPORTLIST)/libportlist.a $(DIRPORTLIST)/portlisttest

unit_PORTLIST: $(DIRPORTLIST)/portlisttest
	$(DIRPORTLIST)/portlisttest

$(DIRPORTLIST)/libportlist.a: $(PORTLIST_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_PORTLIST)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRPORTLIST)/portlisttest: $(DIRPORTLIST)/portlisttest.o $(DIRPORTLIST)/libportlist.a $(MAKEFILES_COMMON) $(MAKEFILES_PORTLIST)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_PORTLIST) -lpthread

$(PORTLIST_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_PORTLIST)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_PORTLIST)
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c $(CFLAGS_PORTLIST)

$(PORTLIST_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_PORTLIST)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_PORTLIST)

clean_PORTLIST:
	rm -f $(PORTLIST_OBJ) $(PORTLIST_DEP)

distclean_PORTLIST: clean_PORTLIST
	rm -f $(DIRPORTLIST)/libportlist.a $(DIRPORTLIST)/portlisttest

-include $(DIRPORTLIST)/*.d
