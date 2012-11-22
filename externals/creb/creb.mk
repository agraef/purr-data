#!/usr/bin/make -f

# Faster alternative makefile, for development only.
SRC_C:= \
./modules/dwt.c \
./modules/scrollgrid1D~.c \
./modules/diag~.c \
./modules/statwav~.c \
./modules/xfm~.c \
./modules/junction~.c \
./modules/permut~.c \
./modules/dynwav~.c \
./modules/dist~.c \
./modules/setup.c \
./modules/qmult~.c \
./modules/ffpoly.c \
./modules/sawtooth~.c \
./modules/sbosc~.c \
./modules/ratio.c \
./modules/bitsplit~.c \
./modules/fdn~.c \
./modules/eblosc~.c \
./modules/bwin~.c \
./modules/fwarp.c \
./modules/cmath.c \
./modules/cheby~.c \
./modules/blocknorm~.c \
./modules/bdiag~.c \
./modules/ramp~.c \
./modules/tabreadmix~.c \
./modules/ead~.c \
./modules/matrix~.c \
./modules/abs~.c \
./modules/lattice~.c \
./modules/resofilt~.c \
./modules/ear~.c \
./modules/qnorm~.c \
./modules/eadsr~.c \
./modules/bfft~.c \

SRC_CC := \
./modules++/blosc~.cc \
./modules++/biquadseries~.cc \
./modules++/filterortho~.cc \


GCC_CFLAGS := -funroll-loops
CC := gcc $(GCC_CFLAGS)
CPLUSPLUS := g++ $(GCC_CFLAGS)
# CC := clang
# CPLUSPLUS := clang++


CFLAGS := -DPD -DCREB_VERSION=\"0.9.2\" -fPIC -O3 -fomit-frame-pointer -Wall -W -Wno-unused -Wno-parentheses -Wno-switch
BUILD := build
ARCH := pd_linux
LDFLAGS := -rdynamic -shared
OUT := $(BUILD)/creb.$(ARCH)

O := \
	$(patsubst %.c,$(BUILD)/%.o,$(SRC_C)) \
	$(patsubst %.cc,$(BUILD)/%.o,$(SRC_CC))
D := $(O:.o=.d)


.SECONDARY:
.DELETE_ON_ERROR:

.PHONY: all
all: $(OUT)

.PHONY: clean
clean:
	rm -rf build

$(BUILD)/%.d: %.c
	@echo [d] $(notdir $@)
	@mkdir -p $(dir $@)
	@$(CC) -MT $(basename $@).o -MM $(CFLAGS) $< >$@

$(BUILD)/%.d: %.cc
	@echo [d] $(notdir $@)
	@mkdir -p $(dir $@)
	@$(CPLUSPLUS) -MT $(basename $@).o -MM $(CFLAGS) $< >$@

$(BUILD)/%.o: %.c $(BUILD)/%.d
	@echo [o] $(notdir $@)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.cc $(BUILD)/%.d
	@echo [o] $(notdir $@)
	@mkdir -p $(dir $@)
	@$(CPLUSPLUS) $(CFLAGS) -c $< -o $@

$(OUT): $(O)
	@echo [pd_linux] $(notdir $@)
	@$(CPLUSPLUS) $(LDFLAGS) -o $@ $(O) $(LIBS)

-include $(D)

