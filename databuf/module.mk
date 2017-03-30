DATABUF_SRC_LIB := databuf.c
DATABUF_SRC := $(DATABUF_SRC_LIB) databuftest.c

DATABUF_SRC_LIB := $(patsubst %,$(DIRDATABUF)/%,$(DATABUF_SRC_LIB))
DATABUF_SRC := $(patsubst %,$(DIRDATABUF)/%,$(DATABUF_SRC))

DATABUF_OBJ_LIB := $(patsubst %.c,%.o,$(DATABUF_SRC_LIB))
DATABUF_OBJ := $(patsubst %.c,%.o,$(DATABUF_SRC))

DATABUF_DEP_LIB := $(patsubst %.c,%.d,$(DATABUF_SRC_LIB))
DATABUF_DEP := $(patsubst %.c,%.d,$(DATABUF_SRC))

CFLAGS_DATABUF :=

MAKEFILES_DATABUF := $(DIRDATABUF)/module.mk

.PHONY: DATABUF clean_DATABUF distclean_DATABUF unit_DATABUF $(LCDATABUF) clean_$(LCDATABUF) distclean_$(LCDATABUF) unit_$(LCDATABUF)

$(LCDATABUF): DATABUF
clean_$(LCDATABUF): clean_DATABUF
distclean_$(LCDATABUF): distclean_DATABUF
unit_$(LCDATABUF): unit_DATABUF

DATABUF: $(DIRDATABUF)/libdatabuf.a $(DIRDATABUF)/databuftest

unit_DATABUF: $(DIRDATABUF)/databuftest
	$(DIRDATABUF)/databuftest

$(DIRDATABUF)/libdatabuf.a: $(DATABUF_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_DATABUF)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRDATABUF)/databuftest: $(DIRDATABUF)/databuftest.o $(DIRDATABUF)/libdatabuf.a $(MAKEFILES_COMMON) $(MAKEFILES_DATABUF)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_DATABUF)

$(DATABUF_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_DATABUF)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_DATABUF)

$(DATABUF_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_DATABUF)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_DATABUF)

clean_DATABUF:
	rm -f $(DATABUF_OBJ) $(DATABUF_DEP)

distclean_DATABUF: clean_DATABUF
	rm -f $(DIRDATABUF)/libdatabuf.a $(DIRDATABUF)/databuftest

-include $(DIRDATABUF)/*.d
