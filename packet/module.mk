PACKET_SRC_LIB :=
PACKET_SRC := $(PACKET_SRC_LIB) packettest.c

PACKET_SRC_LIB := $(patsubst %,$(DIRPACKET)/%,$(PACKET_SRC_LIB))
PACKET_SRC := $(patsubst %,$(DIRPACKET)/%,$(PACKET_SRC))

PACKET_OBJ_LIB := $(patsubst %.c,%.o,$(PACKET_SRC_LIB))
PACKET_OBJ := $(patsubst %.c,%.o,$(PACKET_SRC))

PACKET_DEP_LIB := $(patsubst %.c,%.d,$(PACKET_SRC_LIB))
PACKET_DEP := $(patsubst %.c,%.d,$(PACKET_SRC))

CFLAGS_PACKET := -I$(DIRLINKEDLIST)

MAKEFILES_PACKET := $(DIRPACKET)/module.mk

.PHONY: PACKET clean_PACKET distclean_PACKET unit_PACKET $(LCPACKET) clean_$(LCPACKET) distclean_$(LCPACKET) unit_$(LCPACKET)

$(LCPACKET): PACKET
clean_$(LCPACKET): clean_PACKET
distclean_$(LCPACKET): distclean_PACKET
unit_$(LCPACKET): unit_PACKET

PACKET: $(DIRPACKET)/libpacket.a $(DIRPACKET)/packettest

unit_PACKET: $(DIRPACKET)/packettest
	$(DIRPACKET)/packettest

$(DIRPACKET)/libpacket.a: $(PACKET_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_PACKET)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRPACKET)/packettest: $(DIRPACKET)/packettest.o $(DIRPACKET)/libpacket.a $(MAKEFILES_COMMON) $(MAKEFILES_PACKET)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_PACKET)

$(PACKET_OBJ): %.o: %.c $(PACKET_DEP) $(MAKEFILES_COMMON) $(MAKEFILES_PACKET)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_PACKET)

$(PACKET_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_PACKET)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_PACKET)

clean_PACKET:
	rm -f $(PACKET_OBJ) $(PACKET_DEP)

distclean_PACKET: clean_PACKET
	rm -f $(DIRPACKET)/libpacket.a $(DIRPACKET)/packettest

-include $(DIRPACKET)/*.d
