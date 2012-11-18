all: build

build: build-default-default

profile: build-profile-default

clean: clean-default-default

install: install-default-default

###########################################################

build-all-all: build-release-all build-debug-all

build-release-all: build-release-single build-release-multi build-release-shared

build-debug-all: build-debug-single build-debug-multi build-debug-shared

build-profile-all: build-profile-single build-profile-multi build-profile-shared

build-all-single: build-release-single build-debug-single

build-all-multi: build-release-multi build-debug-multi

build-all-shared: build-release-shared build-debug-shared

###################################################

install-all-all: install-release-all install-debug-all

install-release-all: install-release-single install-release-multi install-release-shared

install-debug-all: install-debug-single install-debug-multi install-debug-shared

install-profile-all: install-profile-single install-profile-multi install-profile-shared

install-all-single: install-release-single install-debug-single

install-all-multi: install-release-multi install-debug-multi

install-all-shared: install-release-shared install-debug-shared

###################################################

clean-all-all: clean-release-all clean-debug-all

clean-release-all: clean-release-single clean-release-multi clean-release-shared

clean-debug-all: clean-debug-single clean-debug-multi clean-debug-shared

clean-profile-all: clean-profile-single clean-profile-multi clean-profile-shared

clean-all-single: clean-release-single clean-debug-single

clean-all-multi: clean-release-multi clean-debug-multi

clean-all-shared: clean-release-shared clean-debug-shared

###########################################################

build-release-single: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=release TARGETTYPE=single _build_

build-debug-single: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=debug TARGETTYPE=single DEBUG=1 _build_

build-profile-single: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=profile TARGETTYPE=single PROFILE=1 _build_

build-release-multi: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=release TARGETTYPE=multi THREADED=1 _build_

build-debug-multi: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=debug TARGETTYPE=multi THREADED=1 DEBUG=1 _build_

build-profile-multi: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=profile TARGETTYPE=multi THREADED=1 PROFILE=1 _build_

build-release-shared: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=release TARGETTYPE=shared SHARED=1 _build_

build-debug-shared: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=debug TARGETTYPE=shared SHARED=1 DEBUG=1 _build_

build-profile-shared: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=profile TARGETTYPE=shared SHARED=1 PROFILE=1 _build_

###########################################################

install-release-single: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=release TARGETTYPE=single _install_

install-debug-single: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=debug TARGETTYPE=single DEBUG=1 _install_

install-profile-single: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=profile TARGETTYPE=single PROFILE=1 _install_

install-release-multi: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=release TARGETTYPE=multi THREADED=1 _install_

install-debug-multi: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=debug TARGETTYPE=multi THREADED=1 DEBUG=1 _install_

install-profile-multi: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=profile TARGETTYPE=multi THREADED=1 PROFILE=1 _install_

install-release-shared: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=release TARGETTYPE=shared SHARED=1 _install_

install-debug-shared: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=debug TARGETTYPE=shared SHARED=1 DEBUG=1 _install_

install-profile-shared: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=profile TARGETTYPE=shared SHARED=1 PROFILE=1 _install_

###########################################################

clean-release-single: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=release TARGETTYPE=single _clean_

clean-debug-single: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=debug TARGETTYPE=single DEBUG=1 _clean_

clean-profile-single: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=profile TARGETTYPE=single PROFILE=1 _clean_

clean-release-multi: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=release TARGETTYPE=multi THREADED=1 _clean_

clean-debug-multi: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=debug TARGETTYPE=multi THREADED=1 DEBUG=1 _clean_

clean-profile-multi: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=profile TARGETTYPE=multi THREADED=1 PROFILE=1 _clean_

clean-release-shared: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=release TARGETTYPE=shared SHARED=1 _clean_

clean-debug-shared: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=debug TARGETTYPE=shared SHARED=1 DEBUG=1 _clean_

clean-profile-shared: config
	$(MAKE) $(MAKE_OPTIONS) TARGETMODE=profile TARGETTYPE=shared SHARED=1 PROFILE=1 _clean_

###########################################################

config: $(USRMAKE) $(SYSCONFIG) $(USRCONFIG) 
