MAKEFILES_LIBPPTK := $(DIRLIBPPTK)/module.mk

LIBS_LIBPPTK := $(DIRALLOC)/liballoc.a $(DIRARP)/libarp.a $(DIRAVLTREE)/libavltree.a $(DIRDATABUF)/libdatabuf.a $(DIRDYNARR)/libdynarr.a $(DIRHASHLIST)/libhashlist.a $(DIRHASHTABLE)/libhashtable.a $(DIRIPFRAG)/libipfrag.a $(DIRIPHASH)/libiphash.a $(DIRIPHDR)/libiphdr.a $(DIRLDP)/libldp.a $(DIRLINKEDLIST)/liblinkedlist.a $(DIRLOG)/liblog.a $(DIRMISC)/libmisc.a $(DIRMYPCAP)/libmypcap.a $(DIRNETMAP)/libnetmap.a $(DIRPACKET)/libpacket.a $(DIRPORTLIST)/libportlist.a $(DIRPORTS)/libports.a $(DIRQUEUE)/libqueue.a $(DIRRANDOM)/librandom.a $(DIRRBTREE)/librbtree.a $(DIRTIMERAVL)/libtimeravl.a $(DIRTIMERLINKHEAP)/libtimerlinkheap.a $(DIRTIMERRB)/libtimerrb.a $(DIRTIMERSKIPLIST)/libtimerskiplist.a $(DIRTIMERWHEEL)/libtimerwheel.a $(DIRTUNTAP)/libtuntap.a

.PHONY: LIBPPTK clean_LIBPPTK distclean_LIBPPTK unit_LIBPPTK $(LCLIBPPTK) clean_$(LCLIBPPTK) distclean_$(LCLIBPPTK) unit_$(LCLIBPPTK)

$(LCLIBPPTK): LIBPPTK
clean_$(LCLIBPPTK): clean_LIBPPTK
distclean_$(LCLIBPPTK): distclean_LIBPPTK
unit_$(LCLIBPPTK): unit_LIBPPTK

LIBPPTK: $(DIRLIBPPTK)/libpptk.so $(DIRLIBPPTK)/libpptk.a

ifeq ($(WITH_ODP),yes)
LDFLAGS_LIBPPTK_DYN += -L$(ODP_DIR)/lib
LDFLAGS_LIBPPTK_DYN += -Wl,-rpath,$(ODP_DIR)/lib
LDFLAGS_LIBPPTK_DYN += -lodp-linux
endif
ifeq ($(WITH_DPDK),yes)
LDFLAGS_LIBPPTK_DYN += -L$(DPDK_LIBDIR)
LDFLAGS_LIBPPTK_DYN += -Wl,--whole-archive
LDFLAGS_LIBPPTK_DYN += -ldpdk
LDFLAGS_LIBPPTK_DYN += -Wl,--no-whole-archive
LDFLAGS_LIBPPTK_DYN += -lnuma
LDFLAGS_LIBPPTK_DYN += -lpcap
LDFLAGS_LIBPPTK_DYN += -lm
endif

unit_LIBPPTK:
	@true

$(DIRLIBPPTK)/libpptk.so: $(LIBS_LIBPPTK)
	$(CC) -shared -fPIC -o $(DIRLIBPPTK)/libpptk.so -Wl,--whole-archive $(filter %.a,$^) -Wl,--no-whole-archive $(LDFLAGS_LIBPPTK_DYN) -lc

$(DIRLIBPPTK)/libpptk.a: $(LIBS_LIBPPTK)
	echo "GROUP ( $(LIBS_LIBPPTK) )" > $(DIRLIBPPTK)/libpptk.a

clean_LIBPPTK:
	@true

distclean_LIBPPTK: clean_LIBPPTK
	rm -f $(DIRLIBPPTK)/libpptk.so $(DIRLIBPPTK)/libpptk.a

-include $(DIRLIBPPTK)/*.d
