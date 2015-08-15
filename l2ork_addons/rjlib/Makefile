#! /usr/bin/make

# available targets:
#   clean: clean everything
#   all: compile objective-c application
#   package: prepare a zip file of the app
#   upload: upload app to a phone
# 
# use "make upload IPHONE=<iphone_ip>" to build and upload to device

# name of the application
TRG=RjDj
# version
VER=0.5
# ip of the phone to install it to
IPHONE?=192.168.1.36
# user name to upload with
IPHONE_USER=mobile

SCENE_LOC=/var/mobile/Applications/RJ/rjlib

RSYNC=rsync
RSYNC_OPTS=-av --delete-after --exclude .svn

upload:
	$(RSYNC) $(RSYNC_OPTS) ./ $(IPHONE_USER)@$(IPHONE):$(SCENE_LOC) || echo "make sure rsync is installed on the phone"
