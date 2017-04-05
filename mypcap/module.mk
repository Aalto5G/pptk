MYPCAP_SRC_LIB := mypcap.c mypcapng.c
MYPCAP_SRC := $(MYPCAP_SRC_LIB) testpcap.c testpcapng.c pcaptong.c pcapngcmp.c pcapcmp.c

MYPCAP_SRC_LIB := $(patsubst %,$(DIRMYPCAP)/%,$(MYPCAP_SRC_LIB))
MYPCAP_SRC := $(patsubst %,$(DIRMYPCAP)/%,$(MYPCAP_SRC))

MYPCAP_OBJ_LIB := $(patsubst %.c,%.o,$(MYPCAP_SRC_LIB))
MYPCAP_OBJ := $(patsubst %.c,%.o,$(MYPCAP_SRC))

MYPCAP_DEP_LIB := $(patsubst %.c,%.d,$(MYPCAP_SRC_LIB))
MYPCAP_DEP := $(patsubst %.c,%.d,$(MYPCAP_SRC))

CFLAGS_MYPCAP := -I$(DIRMISC) -I$(DIRDYNARR) -I$(DIRHASHTABLE) -I$(DIRHASHLIST) -I$(DIRIPHDR)
LIBS_MYPCAP := $(DIRDYNARR)/libdynarr.a $(DIRHASHTABLE)/libhashtable.a

MAKEFILES_MYPCAP := $(DIRMYPCAP)/module.mk

.PHONY: MYPCAP clean_MYPCAP distclean_MYPCAP unit_MYPCAP $(LCMYPCAP) clean_$(LCMYPCAP) distclean_$(LCMYPCAP) unit_$(LCMYPCAP)

$(LCMYPCAP): MYPCAP
clean_$(LCMYPCAP): clean_MYPCAP
distclean_$(LCMYPCAP): distclean_MYPCAP
unit_$(LCMYPCAP): unit_MYPCAP

MYPCAP: $(DIRMYPCAP)/libmypcap.a $(DIRMYPCAP)/testpcap $(DIRMYPCAP)/testpcapng $(DIRMYPCAP)/pcaptong $(DIRMYPCAP)/pcapngcmp $(DIRMYPCAP)/pcapcmp

unit_MYPCAP:
	@true

$(DIRMYPCAP)/libmypcap.a: $(MYPCAP_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_MYPCAP)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRMYPCAP)/testpcap: $(DIRMYPCAP)/testpcap.o $(DIRMYPCAP)/libmypcap.a $(LIBS_MYPCAP) $(MAKEFILES_COMMON) $(MAKEFILES_MYPCAP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_MYPCAP)

$(DIRMYPCAP)/testpcapng: $(DIRMYPCAP)/testpcapng.o $(DIRMYPCAP)/libmypcap.a $(LIBS_MYPCAP) $(MAKEFILES_COMMON) $(MAKEFILES_MYPCAP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_MYPCAP)

$(DIRMYPCAP)/pcaptong: $(DIRMYPCAP)/pcaptong.o $(DIRMYPCAP)/libmypcap.a $(LIBS_MYPCAP) $(MAKEFILES_COMMON) $(MAKEFILES_MYPCAP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_MYPCAP)

$(DIRMYPCAP)/pcapngcmp: $(DIRMYPCAP)/pcapngcmp.o $(DIRMYPCAP)/libmypcap.a $(LIBS_MYPCAP) $(MAKEFILES_COMMON) $(MAKEFILES_MYPCAP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_MYPCAP)

$(DIRMYPCAP)/pcapcmp: $(DIRMYPCAP)/pcapcmp.o $(DIRMYPCAP)/libmypcap.a $(LIBS_MYPCAP) $(MAKEFILES_COMMON) $(MAKEFILES_MYPCAP)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_MYPCAP)

$(MYPCAP_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_MYPCAP)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_MYPCAP)

$(MYPCAP_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_MYPCAP)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_MYPCAP)

clean_MYPCAP:
	rm -f $(MYPCAP_OBJ) $(MYPCAP_DEP)

distclean_MYPCAP: clean_MYPCAP
	rm -f $(DIRMYPCAP)/libmypcap.a $(DIRMYPCAP)/testpcap $(DIRMYPCAP)/testpcapng $(DIRMYPCAP)/pcaptong $(DIRMYPCAP)/pcapngcmp $(DIRMYPCAP)/pcapcmp

-include $(DIRMYPCAP)/*.d
