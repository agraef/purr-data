build-default-default: build-$(BUILDMODE)-$(BUILDTYPE)

build-all-default: build-all-$(BUILDTYPE)

build-release-default: build-release-$(BUILDTYPE)

build-debug-default: build-debug-$(BUILDTYPE)

build-profile-default: build-profile-$(BUILDTYPE)

build-default-all: build-$(BUILDMODE)-all

build-default-single: build-$(BUILDMODE)-single

build-default-multi: build-$(BUILDMODE)-multi

build-default-shared: build-$(BUILDMODE)-shared


install-default-default: install-$(BUILDMODE)-$(BUILDTYPE)

install-all-default: install-all-$(BUILDTYPE)

install-release-default: install-release-$(BUILDTYPE)

install-debug-default: install-debug-$(BUILDTYPE)

install-default-all: install-$(BUILDMODE)-all

install-default-single: install-$(BUILDMODE)-single

install-default-multi: install-$(BUILDMODE)-multi

install-default-shared: install-$(BUILDMODE)-shared


clean-default-default: clean-$(BUILDMODE)-$(BUILDTYPE)

clean-all-default: clean-all-$(BUILDTYPE)

clean-release-default: clean-release-$(BUILDTYPE)

clean-debug-default: clean-debug-$(BUILDTYPE)

clean-default-all: clean-$(BUILDMODE)-all

clean-default-single: clean-$(BUILDMODE)-single

clean-default-multi: clean-$(BUILDMODE)-multi

clean-default-shared: clean-$(BUILDMODE)-shared
