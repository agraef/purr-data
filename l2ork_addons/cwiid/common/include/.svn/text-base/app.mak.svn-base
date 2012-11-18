#Copyright (C) 2007 L. Donnie Smith

OBJECTS = $(SOURCES:.c=.o)
DEPS    = $(SOURCES:.c=.d)

INST_DIR ?= /usr/local/bin

DEST_INST_DIR = $(ROOTDIR)$(INST_DIR)

all: $(APP_NAME)

$(APP_NAME): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LDLIBS)

install: $(APP_NAME)
	install -D $(APP_NAME) $(DEST_INST_DIR)/$(APP_NAME)

clean:
	rm -f $(APP_NAME) $(OBJECTS) $(DEPS)

uninstall:
	rm -f $(DEST_INST_DIR)/$(APP_NAME)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),distclean)
include $(COMMON)/include/dep.mak
-include $(DEPS)
endif
endif

.PHONY: all install uninstall clean
