default: all

# Makefile for the VideoIO plugins
# this will descend to every plugin-directory 
# that has a file named "Makefile"

PLUGIN_MAKEFILES=$(wildcard */Makefile)

PLUGINS=$(PLUGIN_MAKEFILES:/Makefile=)
PLUGINS_CLEAN=$(PLUGINS:=_clean)

MAKEGOALS=$(filter-out $(PLUGINS) all,$(MAKECMDGOALS))

.PHONY: default all $(PLUGINS) $(MAKEGOALS)

#the default: build all plugins

all: $(PLUGINS)

$(PLUGINS):
	make -C $@ $(MAKEGOALS)


#other goals: just pass the goal to the plugin-makes
$(MAKEGOALS): $(PLUGINS)

