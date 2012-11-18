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
	etags ../../../pd/src/*.h *.[ch]
	make etags_`uname -s`

etags_Darwin:
	etags -a \
		/System/Library/Frameworks/ForceFeedback.framework/Headers/*.h \
		/System/Library/Frameworks/CoreFoundation.framework/Headers/*.h \
		/System/Library/Frameworks/Carbon.framework/Headers/*.h  \
		/System/Library/Frameworks/IOBluetooth.framework/Headers/*.[ch]

etags_Linux:
	etags -a /usr/include/*.h linux/input.h /usr/include/sys/*.h

etags_MINGW:
	etags -a /usr/include/*.h /usr/include/sys/*.h \
		/usr/local/include/*.h /usr/local/include/sys/*.h
