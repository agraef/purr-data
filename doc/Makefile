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

DOC_TARGETS = media messageoddness pd_fileformat pd-msg \
tutorials

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
# EXTERNALS-HOWTO
EXTERNALS-HOWTO_NAME = Externals-HOWTO
externals-howto_install: $(manualsdir)
	install -d $(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/pd-externals-HOWTO.pdf > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/pd-externals-HOWTO.pdf
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/HOWTO-externals-en.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/HOWTO-externals-en.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node1.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node1.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node2.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node2.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node3.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node3.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node4.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node4.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node5.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node5.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node6.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node6.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node7.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node7.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node8.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node8.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node9.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node9.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node10.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node10.html

externals-howto_clean:




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




#------------------------------------------------------------------------------#
# MESSAGEODDNESS
MESSAGEODDNESS_NAME = MessageOddness
messageoddness_install: $(manualsdir)
	install -d $(DESTDIR)$(manualsdir)/$(MESSAGEODDNESS_NAME)
	install -p $(doc_src)/additional/messageoddness/*.* \
		$(DESTDIR)$(manualsdir)/$(MESSAGEODDNESS_NAME) 

messageoddness_clean:
	-rm -f -- $(DESTDIR)$(manualsdir)/$(MESSAGEODDNESS_NAME)/*.*
	-rmdir --	$(DESTDIR)$(manualsdir)/$(MESSAGEODDNESS_NAME) 




#------------------------------------------------------------------------------#
# PD-MSG
PD-MSG_NAME = pd-msg
pd-msg_install: $(manualsdir)
	install -d $(DESTDIR)$(manualsdir)/$(PD-MSG_NAME)
	install -p $(doc_src)/additional/pd-msg/*.txt \
		$(DESTDIR)$(manualsdir)/$(PD-MSG_NAME) 
	for dir in $(shell cd  $(doc_src)/additional/pd-msg && ls -d [1-5].*); do \
		echo "Including $$dir in $(PD-MSG_NAME)"; \
		install -d $(DESTDIR)$(manualsdir)/$(PD-MSG_NAME)/$$dir; \
		install -p $(doc_src)/additional/pd-msg/$$dir/*.* \
			$(DESTDIR)$(manualsdir)/$(PD-MSG_NAME)/$$dir; \
	done

pd-msg_clean:


#------------------------------------------------------------------------------#
# PD_FILEFORMAT
PD_FILEFORMAT_NAME = Pd
pd_fileformat_install: $(manualsdir)
	install -d $(DESTDIR)$(manualsdir)/$(PD_FILEFORMAT_NAME)
	install -p $(doc_src)/additional/pd-fileformat.html \
		$(DESTDIR)$(manualsdir)/$(PD_FILEFORMAT_NAME)/Pd_File_Format.html

pd_fileformat_clean:

#------------------------------------------------------------------------------#
# TUTORIALS
tutorials_install: $(manualsdir)
# start-here
	install -d "$(DESTDIR)$(manualsdir)/StartHere"
	install -p $(doc_src)/tutorials/start-here/*.* "$(DESTDIR)$(manualsdir)/StartHere"
# pddrums
	install -d $(DESTDIR)$(manualsdir)/PdDrums
	install -p $(doc_src)/tutorials/footils/pddrums/*.* $(DESTDIR)$(manualsdir)/PdDrums
# playnow
	install -d $(DESTDIR)$(manualsdir)/PlayNow
	install -p $(doc_src)/tutorials/playnow/*.* $(DESTDIR)$(manualsdir)/PlayNow
# cognition
	install -d $(DESTDIR)$(manualsdir)/Cognition
	install -p $(doc_src)/tutorials/cognition/*.* $(DESTDIR)$(manualsdir)/Cognition
# intro
	install -d $(DESTDIR)$(manualsdir)/0.Intro
	install -p $(doc_src)/tutorials/intro/*.* $(DESTDIR)$(manualsdir)/0.Intro
	install -p $(externals_src)/pddp/+pager.pd $(DESTDIR)$(manualsdir)/0.Intro
# sound
	install -d $(DESTDIR)$(manualsdir)/1.Sound
	install -p $(doc_src)/tutorials/sound/*.* $(DESTDIR)$(manualsdir)/1.Sound
	install -p $(externals_src)/pddp/+pager.pd $(DESTDIR)$(manualsdir)/1.Sound
# visual
	install -d $(DESTDIR)$(manualsdir)/2.Image
	install -p $(doc_src)/tutorials/visual/*.* $(DESTDIR)$(manualsdir)/2.Image
	install -p $(externals_src)/pddp/+pager.pd $(DESTDIR)$(manualsdir)/2.Image
# networking
	install -d $(DESTDIR)$(manualsdir)/3.Networking
	install -p $(doc_src)/tutorials/networking/*.* $(DESTDIR)$(manualsdir)/3.Networking
	install -p $(externals_src)/pddp/+pager.pd $(DESTDIR)$(manualsdir)/3.Networking
# physical
#	install -d $(manualsdir)/4.Physical
#	install -p $(doc_src)/tutorials/physical/*.* $(manualsdir)/4.Physical
#	install -p $(externals_src)/pddp/+pager.pd $(manualsdir)/4.Physical

tutorials_clean:
	-rm -f -- $(DESTDIR)$(manualsdir)/PdDrums/*.*
	-rmdir -- $(DESTDIR)$(manualsdir)/PdDrums
	-rm -f -- $(DESTDIR)$(manualsdir)/PlayNow/*.*
	-rmdir -- $(DESTDIR)$(manualsdir)/PlayNow
	-rm -f -- $(DESTDIR)$(manualsdir)/0.Intro/*.*
	-rmdir -- $(DESTDIR)$(manualsdir)/0.Intro
	-rm -f -- $(DESTDIR)$(manualsdir)/1.Sound/*.*
	-rmdir -- $(DESTDIR)$(manualsdir)/1.Sound
	-rm -f -- $(DESTDIR)$(manualsdir)/0.Intro/Visual/*.*
	-rmdir -- $(DESTDIR)$(manualsdir)/0.Intro/Visual
	-rm -f -- $(DESTDIR)$(manualsdir)/3.Networking/*.*
	-rmdir -- $(DESTDIR)$(manualsdir)/3.Networking
	-rm -f -- $(DESTDIR)$(manualsdir)/4.Physical/*.*
	-rmdir -- $(DESTDIR)$(manualsdir)/4.Physical


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
