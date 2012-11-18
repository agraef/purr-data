TARGET := $(shell pwd | sed 's|.*/\(.*\)$$|\1|')
EXTERNALS_ROOT := $(shell pwd | sed 's|^\(/.*externals\).*|\1|')

default: 
	make -C $(EXTERNALS_ROOT) $(TARGET)

install:
	make -C $(EXTERNALS_ROOT) $(TARGET)_install

clean:
	make -C $(EXTERNALS_ROOT) $(TARGET)_clean

test_locations:
	make -C $(EXTERNALS_ROOT) test_locations

# for emacs
etags:
	make etags_`uname -s`

etags_Darwin:
	etags *.[ch] linux/input.h HID\ Utilities\ Source/*.[ch] \
		/System/Library/Frameworks/IOKit.framework/Headers/hid*/*.[ch]

etags_Linux:
	etags *.[ch] /usr/include/*.h linux/input.h /usr/include/sys/*.h
