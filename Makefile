CC := cc
#CC := clang

.SUFFIXES:

DIRDYNARR := dynarr
LCDYNARR := dynarr
MODULES += DYNARR

DIRMISC := misc
LCMISC := misc
MODULES += MISC

DIRHASHLIST := hashlist
LCHASHLIST := hashlist
MODULES += HASHLIST

DIRHASHTABLE := hashtable
LCHASHTABLE := hashtable
MODULES += HASHTABLE

DIRLINKEDLIST := linkedlist
LCLINKEDLIST := linkedlist
MODULES += LINKEDLIST

DIRLOG := log
LCLOG := log
MODULES += LOG

CFLAGS := -g -O2 -Wall -Werror

.PHONY: all clean distclean unit

all: $(MODULES)
clean: $(patsubst %,clean_%,$(MODULES))
distclean: $(patsubst %,distclean_%,$(MODULES))
unit: $(patsubst %,unit_%,$(MODULES))

MAKEFILES_COMMON := Makefile

$(foreach module,$(MODULES),$(eval \
    include $(DIR$(module))/module.mk))
