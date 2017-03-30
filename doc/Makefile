#==============================================================================#
#
# Centralized build system for "doc".  
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

CURL := curl --connect-timeout 600 --max-time 3600

# default target
all:
	@echo "this currently does nothing"

include $(BUILDLAYOUT_DIR)/Makefile.buildlayout

#==============================================================================#
#
# OVERARCHING BUILD TARGETS
#
#==============================================================================#

DOC_TARGETS = media

# clean up after everything is installed
final_setup:
	chmod -R ugo-w $(pddocdir)

install: $(objectsdir) $(helpdir) $(manualsdir) $(examplesdir) \
$(patsubst %, %_install,$(DOC_TARGETS))
	@echo " "
	@echo "doc install succeeded!"


#==============================================================================#
#
# PROJECT-SPECIFIC TARGETS
#
#==============================================================================#


#------------------------------------------------------------------------------#
# TEMPLATE
TEMPLATE_NAME = template
template_install: $(manualsdir)
	install -d $(helpdir)$(manualsdir)/$(TEMPLATE_NAME)
	install -p $(doc_src)/template/*.* \
		$(helpdir)$(manualsdir)/$(TEMPLATE_NAME) 

template_clean:
	-rm -f -- $(helpdir)$(manualsdir)/$(TEMPLATE_NAME)/*.*
	-rmdir --	$(helpdir)$(manualsdir)/$(TEMPLATE_NAME) 


#------------------------------------------------------------------------------#
# MEDIA
MEDIA_NAME = media
media_install: $(pddocdir)
	install -d $(DESTDIR)$(pddocdir)/$(MEDIA_NAME)
	install -p $(doc_src)/media/*.* \
		$(DESTDIR)$(pddocdir)/$(MEDIA_NAME) 
# ln is cp on MinGW, so these won't work since the target files don't exist at
# the time that the media_install target is generally run. For MinGW, this is
# instead handled in packages/win32_inno/pd-inno.iss.in
ifneq (MINGW,$(findstring MINGW,$(UNAME)))
# random sound files
	ln -sf ../../doc/sound/bell.aiff \
		$(DESTDIR)$(pddocdir)/$(MEDIA_NAME)/bell.aiff
	ln -sf ../../doc/sound/voice.wav \
		$(DESTDIR)$(pddocdir)/$(MEDIA_NAME)/voice.wav
	ln -sf ../../doc/sound/voice2.wav \
		$(DESTDIR)$(pddocdir)/$(MEDIA_NAME)/voice2.wav
	ln -sf ../../extra/bsaylor/examples/noiseburst.wav \
		$(DESTDIR)$(pddocdir)/$(MEDIA_NAME)/noiseburst.wav
	ln -sf ../../extra/ekext/examples/stink.wav \
		$(DESTDIR)$(pddocdir)/$(MEDIA_NAME)/stink.wav
	ln -sf ../../extra/ekext/examples/beauty.wav \
		$(DESTDIR)$(pddocdir)/$(MEDIA_NAME)/beauty.wav
	ln -sf ../../extra/ekext/examples/drummach.wav \
		$(DESTDIR)$(pddocdir)/$(MEDIA_NAME)/drummach.wav
# Gem videos
	ln -sf ../../extra/Gem/examples/data/alea.mpg \
		$(DESTDIR)$(pddocdir)/$(MEDIA_NAME)/alea.mpg
	ln -sf ../../extra/Gem/examples/data/homer.avi \
		$(DESTDIR)$(pddocdir)/$(MEDIA_NAME)/homer.avi
	ln -sf ../../extra/Gem/examples/data/anim-1.mov \
		$(DESTDIR)$(pddocdir)/$(MEDIA_NAME)/anim-1.mov
endif

media_clean:
	-rm -f -- $(DESTDIR)$(pddocdir)/$(MEDIA_NAME)/*.*
	-rmdir --	$(DESTDIR)$(pddocdir)/$(MEDIA_NAME) 

#==============================================================================#
#
# CLEAN TARGETS
#
#==============================================================================#

# the destination-specific clean targets are in Makefile.buildlayout
clean: $(patsubst %, %_clean,$(DOC_TARGETS))

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
