#############################################################
# videoIO makefile
#
# 2008, Georg Holzmann
# 2008, IOhannes m zmölnig
#############################################################

-include Make.config

#############################################################
# build gem-external targets and objects

.PHONY: plugins core gem

# all builds gem-externals and plugins (gstreamer)
all: core gem plugins

gem: core
	make -C gem

core:
	make -C core

plugins: 
	make -C plugins


#############################################################
# clean and install

clean:
	make -C plugins clean
	make -C gem clean
	make -C core clean

install:
	make -C gem install
	make -C plugins install

# EOF
#############################################################
