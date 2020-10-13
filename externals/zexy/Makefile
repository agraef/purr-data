# Makefile to build 'zexy' for Pure Data.
# Needs Makefile.pdlibbuilder as helper makefile for platform-dependent build
# settings and rules.

# library name
lib.name = zexy


make-lib-executable=no
with-regex=yes
with-lpt=yes

# input source file (class name == source file basename)
class.sources = \
	src/0x260x260x7e.c \
	src/0x2e.c \
	src/0x3c0x7e.c \
	src/0x3d0x3d0x7e.c \
	src/0x3e0x7e.c \
	src/0x7c0x7c0x7e.c \
	src/a2l.c \
	src/abs~.c \
	src/absgn~.c \
	src/atof.c \
	src/atoi.c \
	src/avg~.c \
	src/blockmirror~.c \
	src/blockshuffle~.c \
	src/blockswap~.c \
	src/date.c \
	src/demultiplex~.c \
	src/demultiplex.c \
	src/dfreq~.c \
	src/dirac~.c \
	src/doublepole~.c \
	src/drip.c \
	src/envrms~.c \
	src/fifop.c \
	src/freadln.c \
	src/fwriteln.c \
	src/glue.c \
	src/index.c \
	src/length.c \
	src/lifop.c \
	src/limiter~.c \
	src/list2int.c \
	src/list2lists.c \
	src/list2symbol.c \
	src/lister.c \
	src/listfind.c \
	src/liststorage.c \
	src/longload.c \
	src/lpt.c \
	src/makesymbol.c \
	src/matchbox.c \
	src/mavg.c \
	src/minmax.c \
	src/msgfile.c \
	src/multiline~.c \
	src/multiplex~.c \
	src/multiplex.c \
	src/multireceive.c \
	src/niagara.c \
	src/noish~.c \
	src/noisi~.c \
	src/operating_system.c \
	src/pack~.c \
	src/pack.c \
	src/packel.c \
	src/pdf~.c \
	src/prime.c \
	src/quantize~.c \
	src/rawprint.c \
	src/regex.c \
	src/relay.c \
	src/repack.c \
	src/repeat.c \
	src/route~.c \
	src/sfplay.c \
	src/sfrecord.c \
	src/sgn~.c \
	src/sigzero~.c \
	src/sleepgrain.c \
	src/sort.c \
	src/step~.c \
	src/strcmp.c \
	src/sum.c \
	src/swap~.c \
	src/symbol2list.c \
	src/tabdump.c \
	src/tabminmax.c \
	src/tabread4~~.c \
	src/tabset.c \
	src/tavg~.c \
	src/time.c \
	src/unpack~.c \
	src/unpack.c \
	src/urn.c \
	src/wrap.c \
	src/z~.c \
	$(empty)

lib.setup.sources = \
	src/zexy.c \
	src/z_zexy.c \
	$(empty)

## TODO: install help-files for these aliases
l.class.sources = src/lister.c
demux~.class.sources = src/demultiplex~.c
demux.class.sources = src/demultiplex.c
mux~.class.sources = src/multiplex~.c
mux.class.sources = src/multiplex.c
l2s.class.sources = src/list2symbol.c
s2l.class.sources = src/symbol2list.c
l2i.class.sources = src/list2int.c
any2list.class.sources = src/a2l.c

# abs-aliases: any2list, l2i

# all extra files to be included in binary distribution of the library
datafiles = \
	AUTHORS \
	README.txt \
	LICENSE.txt \
	ChangeLog \
	zexy-meta.pd \
	$(empty)

datafiles += \
	$(wildcard abs/*.pd) \
	$(wildcard reference/*.pd) \
	$(empty)

cflags = -DVERSION='"$(lib.version)"'

DATE_FMT = %Y/%m/%d at %H:%M:%S UTC
ifdef SOURCE_DATE_EPOCH
    BUILD_DATE ?= $(shell date -u -d "@$(SOURCE_DATE_EPOCH)" "+$(DATE_FMT)" 2>/dev/null || date -u -r "$(SOURCE_DATE_EPOCH)" "+$(DATE_FMT)" 2>/dev/null || date -u "+$(DATE_FMT)")
endif
ifdef BUILD_DATE
cflags += -DBUILD_DATE='"$(BUILD_DATE)"'
endif


ifeq ($(make-lib-executable),yes)
 cflags += -DZEXY_LIBRARY
endif

ifeq ($(with-regex),yes)
 cflags += -DHAVE_REGEX_H
endif

ifneq ($(with-lpt),yes)
 cflags += -DZ_WANT_LPT=0
endif

define forWindows
 ifeq ($(with-regex),yes)
   regex.class.ldlibs += -lregex
   matchbox.class.ldlibs += -lregex
   ifeq ($(make-lib-executable),yes)
     ldlibs += -lregex
   endif
 endif
endef

define forLinux
 # on linux we need <sys/io.h> for the [lpt] object, so check if it is there...
 ifeq ($(with-lpt),yes)
  ifeq ($(shell $(CPP) -x c -include "sys/io.h" /dev/null >/dev/null 2>&1 || echo no), no)
    cflags += -DZ_WANT_LPT=0
  endif
 endif
endef



# include Makefile.pdlibbuilder from submodule directory 'pd-lib-builder'
PDLIBBUILDER_DIR=pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder

COPY=cp
# aliases
vpath %.pd abs reference
zexyaliases = \
	any2list.pd any2list-help.pd \
	demux-help.pd demux~-help.pd \
	mux-help.pd mux~-help.pd \
	l2i.pd l2i-help.pd \
	l2s-help.pd s2l-help.pd l-help.pd \
	$(empty)
datafiles += $(zexyaliases)

# create aliases
$(zexyaliases):
	test -e $< && $(COPY) $< $@

# delete aliases
.PHONY: clean_zexyalias
clean_zexyalias:
	-rm -f $(zexyaliases)
clean: clean_zexyalias

# alias dependencies
any2list.pd: a2l.pd
any2list-help.pd: a2l-help.pd
l2i.pd: list2int.pd
l2i-help.pd: list2int-help.pd
l2s-help.pd: list2symbol-help.pd
s2l-help.pd: symbol2list-help.pd
l-help.pd: lister-help.pd
mux-help.pd: multiplex-help.pd
mux~-help.pd: multiplex~-help.pd
demux-help.pd: demultiplex-help.pd
demux~-help.pd: demultiplex~-help.pd

all: $(zexyaliases)

.PHONY: check style

style:
	astyle --options=src/astyle.rc src/*.c src/*.h

check: all
	LIBDIR=$(CURDIR) find tests/*/ -type f -name "*.pd" -exec tests/testrunner.sh -v -Xls {} "+"
