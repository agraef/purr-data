#!/bin/bash
# nwjs_version_for_platform.sh
#
# Determine the version of nw.js that Purr Data should use on a given platform.
#
# Variables "$os" and "$arch" can be set in the environment, or autodetected.

set -e

if [ -z "$os" ]; then
	os=`uname | tr '[:upper:]' '[:lower:]'`
	if [[ $os == *"mingw32"* ]]; then
		os=win
	elif [[ $os == "darwin" ]]; then
		os=osx
	fi
fi

if [ -z "$arch" ]; then
	if [ `getconf LONG_BIT` -eq 32 ]; then
		arch="ia32"
	else
		arch="x64"
	fi

	# for rpi
	if [ `uname -m` == "armv7l" ]; then
		arch="armv7l"
	fi

	# for pinebook, probably also rpi 4
	if [ `uname -m` == "aarch64" ]; then
		arch="armv7l"
	fi
fi

# MSYS: Pick the right architecture depending on whether we're
# running in the 32 or 64 bit version of the MSYS shell.
if [[ $os == "win" ]]; then
	arch="ia32"
elif [[ $os == "win64" ]]; then
	arch="x64"
fi

if [[ $os == "win" || $os == "win64" || $os == "osx" ]]; then
	ext="zip"
else
	ext="tar.gz"
fi

if [[ $osx_version == "10.8" ]]; then
	# We need the lts version to be able to run on legacy systems.
	nwjs_version="v0.14.7"
else
	# temporary kluge for rpi-- only 0.15.1 is available atm
	if [ $arch == "armv7l" ]; then
		nwjs_version="v0.17.6"
	else
		nwjs_version="v0.24.4"
	fi
fi

nwjs="nwjs-sdk"
if [[ $os == "win64" ]]; then
	nwjs_dirname=${nwjs}-${nwjs_version}-win-${arch}
else
	nwjs_dirname=${nwjs}-${nwjs_version}-${os}-${arch}
fi
nwjs_filename=${nwjs_dirname}.${ext}

echo $nwjs_filename
