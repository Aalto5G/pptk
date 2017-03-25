DIRDYNARR := dynarr
LCDYNARR := dynarr
MODULES += DYNARR

DIRMISC := misc
LCMISC := misc
MODULES += MISC

CFLAGS := -g

.PHONY: all clean distclean unit

all: $(MODULES)
clean: $(patsubst %,clean_%,$(MODULES))
distclean: $(patsubst %,distclean_%,$(MODULES))
unit: $(patsubst %,unit_%,$(MODULES))

$(foreach module,$(MODULES),$(eval \
    include $(DIR$(module))/module.mk))
