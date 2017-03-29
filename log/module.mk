LOG_SRC_LIB := log.c
LOG_SRC := $(LOG_SRC_LIB) logtest.c

LOG_SRC_LIB := $(patsubst %,$(DIRLOG)/%,$(LOG_SRC_LIB))
LOG_SRC := $(patsubst %,$(DIRLOG)/%,$(LOG_SRC))

LOG_OBJ_LIB := $(patsubst %.c,%.o,$(LOG_SRC_LIB))
LOG_OBJ := $(patsubst %.c,%.o,$(LOG_SRC))

LOG_DEP_LIB := $(patsubst %.c,%.d,$(LOG_SRC_LIB))
LOG_DEP := $(patsubst %.c,%.d,$(LOG_SRC))

CFLAGS_LOG := -I$(DIRHASHLIST) -I$(DIRMISC)

MAKEFILES_LOG := $(DIRLOG)/module.mk

.PHONY: LOG clean_LOG distclean_LOG unit_LOG $(LCLOG) clean_$(LCLOG) distclean_$(LCLOG) unit_$(LCLOG)

$(LCLOG): LOG
clean_$(LCLOG): clean_LOG
distclean_$(LCLOG): distclean_LOG
unit_$(LCLOG): unit_LOG

LOG: $(DIRLOG)/liblog.a $(DIRLOG)/logtest

unit_LOG: $(DIRLOG)/logtest
	$(DIRLOG)/logtest

$(DIRLOG)/liblog.a: $(LOG_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_LOG)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRLOG)/logtest: $(DIRLOG)/logtest.o $(DIRLOG)/liblog.a $(MAKEFILES_COMMON) $(MAKEFILES_LOG)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_LOG)

$(LOG_OBJ): %.o: %.c $($*.d) $(MAKEFILES_COMMON) $(MAKEFILES_LOG)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_LOG)

$(LOG_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_LOG)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_LOG)

clean_LOG:
	rm -f $(LOG_OBJ) $(LOG_DEP)

distclean_LOG: clean_LOG
	rm -f $(DIRLOG)/liblog.a $(DIRLOG)/logtest

-include $(DIRLOG)/*.d
