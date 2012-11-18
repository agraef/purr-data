#==============================================================================#
#
# Centralized build system for "extensions".  
#
# see for instructions: http://puredata.org/docs/developer/build
#  <hans@at.or.at>
#
#==============================================================================#

CWD := $(shell pwd)

# these are designed to be overridden by the packages/Makefile
cvs_root_dir := $(shell cd $(CWD)/.. && pwd)
DESTDIR = $(CWD)/build/
BUILDLAYOUT_DIR = $(cvs_root_dir)/packages

# default target
all: gripd

include $(BUILDLAYOUT_DIR)/Makefile.buildlayout

#==============================================================================#
#
# OVERARCHING BUILD TARGETS
#
#==============================================================================#

# clean up after everything is installed
final_setup:
	chmod -R ugo-w $(pddocdir)

install: $(examplesdir) $(helpdir) $(manualsdir) $(objectsdir) $(readmesdir) \
gripd_install ix_install
	@echo " "
	@echo "extensions install succeeded!"

#==============================================================================#
#
# PROJECT-SPECIFIC TARGETS
#
#==============================================================================#

#------------------------------------------------------------------------------#
# GRIPD
GRIPD_NAME = gripd
gripd:
	make -C $(extensions_src)/gripd/src -f Makefile.Linux all

gripd_install: 
	install -d $(helpdir)
	install -d $(objectsdir)
# it doesn't compile automatically yet <hans@at.or.at>
#	install -p $(extensions_src)/gripd/*.$(EXTENSION) \
#		$(extensions_src)/gripd/*.so \
#		$(objectsdir)
	install -p $(extensions_src)/gripd/examples/gripd.pd \
		$(helpdir)/gripd-help.pd
#	install -d $(manualsdir)/$(GRIPD_NAME)
	install -p $(extensions_src)/gripd/README.txt \
		$(readmesdir)/$(GRIPD_NAME).txt
	install -d $(examplesdir)/$(GRIPD_NAME)
	install -p $(extensions_src)/gripd/examples/*.* \
		$(examplesdir)/$(GRIPD_NAME) 


gripd_clean:
	make -C $(extensions_src)/gripd/src clean



#------------------------------------------------------------------------------#
# IX
IX_NAME = ix
ix_install:
	install -d $(objectsdir)/$(IX_NAME)
	install -p $(extensions_src)/gui/ix/toxy/*.wid $(objectsdir)/$(IX_NAME) 
#	install -d $(manualsdir)/$(IX_NAME)
#	install -d $(readmesdir)/$(IX_NAME)
#	install -p $(extensions_src)/gui/ix/toxy/README \
#		$(readmesdir)/$(IX_NAME).txt
	install -d $(examplesdir)/$(IX_NAME)
	install -p $(extensions_src)/gui/ix/toxy/*.pd $(examplesdir)/$(IX_NAME) 



#==============================================================================#
#
# DEVELOPER'S TARGETS
#
#==============================================================================#

# make the symlinks necessary to simulate the installed environment
devsymlinks:

#==============================================================================#
#
# CLEAN TARGETS
#
#==============================================================================#

# the destination-specific clean targets are in Makefile.buildlayout
clean: install_clean

distclean: clean cruft_clean





test_locations:
	@echo "PD_VERSION: $(PD_VERSION)"
	@echo "PACKAGE_VERSION: $(PACKAGE_VERSION)"
	@echo "CWD $(CWD)"
	@echo "DESTDIR $(DESTDIR)"
	@echo "PREFIX $(prefix)"
	@echo "BINDIR  $(bindir)"
	@echo "LIBDIR  $(libdir)"
	@echo "OBJECTSDIR  $(objectsdir)"
	@echo "PDDOCDIR  $(pddocdir)"
	@echo "LIBPDDIR  $(libpddir)"
	@echo "LIBPDBINDIR  $(libpdbindir)"
	@echo "HELPDIR  $(helpdir)"
	@echo "MANUALSDIR  $(manualsdir)"
	@echo "EXAMPLESDIR  $(examplesdir)"
