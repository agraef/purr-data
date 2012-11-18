#Copyright (C) 2007 Justin M. Tulloss

include ../defs.mak

ifdef ROOTDIR
	SET_ROOT_DIR = "--root=$(ROOTDIR)"
endif

all:
	$(PYTHON) setup.py build_ext $(DEBUGFLAGS) -I../libcwiid -L../libcwiid -lcwiid

install:
	$(PYTHON) setup.py install --install-lib=${libdir}/python2.7/site-packages $(SET_ROOT_DIR)

uninstall:
	rm -f $(ROOTDIR)${libdir}/python2.7/site-packages/cwiid.so

clean:
	rm -rf build

distclean: clean
	rm Makefile

.PHONY: all install uninstall clean distclean
