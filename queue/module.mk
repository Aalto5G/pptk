QUEUE_SRC_LIB := queue.c
QUEUE_SRC := $(QUEUE_SRC_LIB) queueperf.c

QUEUE_SRC_LIB := $(patsubst %,$(DIRQUEUE)/%,$(QUEUE_SRC_LIB))
QUEUE_SRC := $(patsubst %,$(DIRQUEUE)/%,$(QUEUE_SRC))

QUEUE_OBJ_LIB := $(patsubst %.c,%.o,$(QUEUE_SRC_LIB))
QUEUE_OBJ := $(patsubst %.c,%.o,$(QUEUE_SRC))

QUEUE_DEP_LIB := $(patsubst %.c,%.d,$(QUEUE_SRC_LIB))
QUEUE_DEP := $(patsubst %.c,%.d,$(QUEUE_SRC))

CFLAGS_QUEUE := -I$(DIRHASHLIST) -I$(DIRMISC)

MAKEFILES_QUEUE := $(DIRQUEUE)/module.mk

.PHONY: QUEUE clean_QUEUE distclean_QUEUE unit_QUEUE $(LCQUEUE) clean_$(LCQUEUE) distclean_$(LCQUEUE) unit_$(LCQUEUE)

$(LCQUEUE): QUEUE
clean_$(LCQUEUE): clean_QUEUE
distclean_$(LCQUEUE): distclean_QUEUE
unit_$(LCQUEUE): unit_QUEUE

QUEUE: $(DIRQUEUE)/libqueue.a $(DIRQUEUE)/queueperf

unit_QUEUE: $(DIRQUEUE)/queueperf
	$(DIRQUEUE)/queueperf

$(DIRQUEUE)/libqueue.a: $(QUEUE_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_QUEUE)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRQUEUE)/queueperf: $(DIRQUEUE)/queueperf.o $(DIRQUEUE)/libqueue.a $(MAKEFILES_COMMON) $(MAKEFILES_QUEUE)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_QUEUE) -lpthread

$(QUEUE_OBJ): %.o: %.c $(QUEUE_DEP) $(MAKEFILES_COMMON) $(MAKEFILES_QUEUE)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_QUEUE)

$(QUEUE_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_QUEUE)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_QUEUE)

clean_QUEUE:
	rm -f $(QUEUE_OBJ) $(QUEUE_DEP)

distclean_QUEUE: clean_QUEUE
	rm -f $(DIRQUEUE)/libqueue.a $(DIRQUEUE)/queueperf

-include $(DIRQUEUE)/*.d
