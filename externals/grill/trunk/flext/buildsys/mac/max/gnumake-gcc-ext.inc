EXT=mxo

##############################################

LDFLAGS += -bundle

##############################################

OBJPATH=$(OUTPATH)/$(OUTSUB)

# bundle name (build site)
INSTTARGET=$(OBJPATH)/$(OUTNAME).$(EXT)
# folder inside bundle (build site)
TARGETPATH=$(INSTTARGET)/Contents/MacOS
# file inside bundle (build site)
TARGET=$(TARGETPATH)/$(OUTNAME)

# post build target
TARGETPOST=$(INSTTARGET)/Contents/Pkginfo

$(INSTTARGET)/Contents/Pkginfo:
	echo "iLaX????" >> $@

