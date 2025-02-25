#!/bin/bash
# super-simplistic installer for l2ork things by Ivica Ico Bukvic <ico@vt.edu>
# for info on L2Ork visit http://l2ork.music.vt.edu

cleanup() {
    # maybe we'd want to do some actual cleanup here
    test $2 -ne 0 && echo "$0: $1: command failed with exit code $2, exiting now." && echo "$0: $1: $BASH_COMMAND"
    exit $2
}

trap 'cleanup $LINENO $?' ERR

if [ $# -eq 0 ] # should check for no arguments
then
	echo
	echo "   Usage: ./tar_em_up.sh -option1 -option2 ..."
	echo "   Options:"
	echo "     -b    build a Debian package (incremental)"
	echo "     -B    build a Debian package (complete recompile)"
	echo "     -c    core Pd source tarball"
	echo "     -f    full tarball installer (incremental)"
	echo "     -F    full tarball installer (complete recompile)"
	echo "     -k    keep previous build products"
	echo "     -l    do a light build (only essential externals)"
	echo "     -n    skip package creation (-bB, -fF)"
	echo "     -r    build a Raspberry Pi deb (incremental)"
	echo "     -R    build a Raspberry Pi deb (complete recompile)"
	echo "     -t    auto-detect target (incremental)"
	echo "     -T    auto-detect target (complete recompile)"
	echo "     -X    build an OSX installer (dmg)"
	echo "     -z    build a Windows installer (incremental)"
	echo "     -Z    build a Windows installer (complete recompile)"
	echo
	echo "   The incremental options bypass Gem compilation. This saves"
	echo "   (lots of) time, but the generated package will lack Gem"
	echo "   unless it has already been built previously."
	echo
	echo "   The -k (keep) option doesn't clean before compilation,"
	echo "   preserving the build products from a previous run. This"
	echo "   also saves time if the script has been run previously."
	echo "   Not using this option forces a full recompile."
	echo
	echo "   For custom install locations and staged installations"
	echo "   set the inst_dir environment variable as follows:"
	echo
	echo "           export inst_dir=/some/custom/location"
	echo
	exit 1
fi

deb=0
core=0
full=0
rpi=0
pkg=1
inno=0
dmg=0
any=0
clean=1
light=0

while getopts ":bBcfFklnRrTtXzZ" Option
do case $Option in
		b)		deb=1
				inst_dir=${inst_dir:-/usr};;

		B)		deb=2
				inst_dir=${inst_dir:-/usr};;

		c)		core=1;;

		f)		full=1;;

		F)		full=2;;

		k)		clean=0;;

		l)		light=1;;

		n)		pkg=0;;

		R)		deb=2
				inst_dir=/usr
				rpi=1;;

		r)		deb=1
				inst_dir=/usr
				rpi=1;;

		t)		any=1;;

		T)		any=2;;

		X)		dmg=1
				inst_dir=/usr;;

		z)		inno=1
				inst_dir=/usr;;

		Z)		inno=2
				inst_dir=/usr;;

		*)		echo "Error: unknown option";;
	esac
done

inst_dir=${inst_dir:-/usr/local}
dpkg=${dpkg:-/usr/bin/dpkg-deb}

# configure a light build if requested
if [ $light -gt 0 ]; then
    export LIGHT=yes
else
    export LIGHT=
fi

export TAR_EM_UP_PREFIX=$inst_dir

# Get the OS we're running under, normalized to names that can be used
# to fetch the nwjs binaries below

os=`uname | tr '[:upper:]' '[:lower:]'`
if [[ $os == *"mingw64"* ]]; then
	os=win64
elif [[ $os == *"mingw"* ]]; then
	os=win
elif [[ $os == "darwin" ]]; then
	os=osx
fi

# Auto-detect the platform and pick an appropriate build target.
if [ $any -gt 0 ]; then
	if [[ $os == "osx" ]]; then
		dmg=$any
	elif [[ $os == "win" || $os == "win64" ]]; then
		inno=$any
	else
		deb=$any
		inst_dir=${inst_dir:-/usr}
	fi
fi

# Make sure that we don't try to build a tarball on Mac or Windows (that's
# part of packages/linux_make and hence only works on Linux), build a regular
# package for the platform instead.
if [ $full -gt 0 ]; then
	if [[ $os == "osx" ]]; then
		dmg=$full
		echo "Warning: tarball installer not supported on Mac, building a dmg installer instead."
	elif [[ $os == "win" || $os == "win64" ]]; then
		inno=$full
		echo "Warning: tarball installer not supported on Windows, building a Windows installer instead."
	fi
	full=0
fi

# Automagically disable Debian packaging when the Debian packaging tools are
# not available.
if test $deb -gt 0 && test $pkg -gt 0 && ! test -x "$dpkg"; then
    pkg=0;
    echo "Debian toolchain unavailable, skipping Debian packaging"
fi

# Fetch the nw.js binary if we haven't already. We want to fetch it even
# for building with no libs, so we do it regardless of the options

# If the nwjsver env variable is set, it denotes the nw.js version for the
# default Linux, Mac, and Windows builds. This cleans out any existing
# binaries, and provides a quick way to switch nw.js versions without having
# to edit this script. For the other platforms, or if the variable isn't set,
# we use a hard-coded default, see below. (Even in that case you can set
# nwjsver to have the cached binaries cleared beforehand.)

if [ -n "$nwjsver" ]; then
	rm -rf "../pd/nw/nw"
fi

if [ ! -d "../pd/nw/nw" ]; then
	if [ `getconf LONG_BIT` -eq 32 ]; then
		arch="ia32"
	else
		arch="x64"
	fi

	# for rpi
	if [ `uname -m` == "armv7l" ]; then
		arch="arm"
	fi

	# for pinebook, probably also rpi 4
	if [ `uname -m` == "aarch64" ]; then
		arch="arm64"
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

	# Some of the nw.js versions are hard-coded for special support of
	# legacy systems, or platforms for which there are no official
	# binaries, so we have to use what we can get from 3rd parties. Some
	# of these are quite old, so expect some gui regressions.
	if [[ $osx_version == "10.8" ]]; then
		# We need the lts version to be able to run on legacy systems.
		nwjs_version="v0.14.7"
	elif [ $os == "osx" ]; then
		# ag: We need a fairly recent version here to make the build
		# work on newer macOS versions. Note that at present only
		# Intel builds are supported, but these should also work on
		# Apple Silicon via Rosetta 2.
		nwjs_version="v${nwjsver:-0.55.0}"
	elif [[ $os == "win" || $os == "win64" ]]; then
		# same version works on Windows, too
		nwjs_version="v${nwjsver:-0.55.0}"
	elif [ $arch == "arm" ]; then
		# rpi-- only 0.27.6 is available atm
		nwjs_version="v0.27.6"
	elif [ $arch == "arm64" ]; then
		# dito for rpi arm64-- 0.23.7 version
		nwjs_version="v0.23.7"
	else
		# default for Linux -- same as Mac and Windows; note that
		# anything past 0.55.0 suffers from the nwworkingdir bug which
		# makes file dialogs open in the wrong locations
		nwjs_version="v${nwjsver:-0.55.0}"
	fi

	nwjs="nwjs-sdk"
	if [[ $os == "win64" ]]; then
		nwjs_dirname=${nwjs}-${nwjs_version}-win-${arch}
	else
		nwjs_dirname=${nwjs}-${nwjs_version}-${os}-${arch}
	fi
	# the arm64 package has a slightly different name
	if [ $arch == "arm64" ]; then
		nwjs_filename=${nwjs}-without-nacl-${nwjs_version}-${os}-${arch}.${ext}
	else
		nwjs_filename=${nwjs_dirname}.${ext}
	fi
	nwjs_url=https://git.purrdata.net/jwilkes/nwjs-binaries/raw/master
	nwjs_url=${nwjs_url}/$nwjs_filename
	echo "Fetching the nwjs binary from"
	echo "$nwjs_url"
	if ! wget -nv $nwjs_url; then
	    if [[ $arch == "arm" || $arch == "arm64" ]]; then
		nwjs_url=https://github.com/LeonardLaszlo/nw.js-armv7-binaries/releases/download/${nwjs_version}/$nwjs_filename
	    else
		nwjs_url=https://dl.nwjs.io/${nwjs_version}/$nwjs_filename
	    fi
	    echo "Fetching the nwjs binary from"
	    echo "$nwjs_url"
	    wget -nv $nwjs_url
	fi
	if [[ $os == "win" || $os == "win64" || $os == "osx" ]]; then
		unzip $nwjs_filename
	else
		tar -xf $nwjs_filename
	fi
        mv $nwjs_dirname ../pd/nw/nw
	# make sure the nw binary is executable on GNU/Linux
	if [[ $os != "win" && $dmg == 0 ]]; then
		chmod 755 ../pd/nw/nw/nw
	fi
	rm $nwjs_filename
fi

# For Windows, fetch the ASIO SDK if we don't have it already
if [[ $os == "win" || $os == "win64" ]]; then
	if [ ! -d "../pd/lib" ]; then
		mkdir ../pd/lib
		wget https://www.steinberg.net/asiosdk -O asiosdk2.3.zip
		unzip asiosdk2.3.zip
		mv asiosdk_2.3* asiosdk2.3 && mv asiosdk2.3 ../pd/lib/
	fi
fi

cd ../

if [ $core -eq 1 ]
then
	echo "core Pd..."
	rm -f ../Pd-l2ork-`date +%Y%m%d`.tar.bz2 2> /dev/null
	cd pd/src/
	# make sure that Pd is configured before trying to package it
	test -f config.h || (aclocal && autoconf && make -C ../../packages pd)
	make clean
	cd ../../
	tar -jcf ./Pd-l2ork-`date +%Y%m%d`.tar.bz2 pd
fi

if [ $full -gt 0 -o $deb -gt 0 -o $inno -gt 0 -o $dmg -gt 0 ]
then
	echo "Pd-L2Ork full installer... IMPORTANT! To ensure you have the most up-to-date submodules, this process requires internet connection to pull sources from various repositories..."

	if [ -d .git ]; then
		git submodule update --init
	fi


	if [ $full -eq 2 -o $deb -eq 2 -o $inno -eq 2 -o $dmg -eq 2 ]
	then
		# We bypass -k when doing a full build for the first time, so
		# that things are set up properly in preparation of the build.
		if [ ! -f Gem/configure ]; then clean=1; fi
		if [ $clean -eq 0 ]; then
		cd externals
		else
		# clean files that may remain stuck even after doing global make clean (if any)
		test $os == "osx" && make -C packages/darwin_app clean || true
		cd externals/miXed
		make clean || true # this may fail on 1st attempt
		cd ../
		make gem_clean || true # this may fail on 1st attempt
		cd ../Gem/src/
		make distclean || true # this may fail on 1st attempt
		rm -rf ./.libs
		rm -rf ./*/.libs
		cd ../
		make distclean || true # this may fail on 1st attempt
		rm -f gemglutwindow.pd_linux
		rm -f Gem.pd_linux
		aclocal
		./autogen.sh
		fi
		export INCREMENTAL=""
	else
		cd Gem/
		export INCREMENTAL="yes"
	fi
	cd ../pd/src && aclocal && autoconf
	if [[ $os == "win" ]]; then
		cd ../../packages/win32_inno
	elif [[ $os == "win64" ]]; then
		cd ../../packages/win64_inno
	elif [[ $os == "osx" ]]; then
		cd ../../packages/darwin_app
	else
		cd ../../packages/linux_make
	fi
	if [ $full -gt 1 -o $deb -eq 2 -o $inno -eq 2 -o $dmg -eq 2 ]
	then
		test $clean -ne 0 && make distclean || true
		# Run `make git_version` *now* so that we already have
		# s_stuff.h when we copy it below. XXXNOTE AG: The build seems
		# to work just fine even when skipping all this, so why again
		# is this needed?
		test -f ../../pd/src/s_version.h || make -C .. git_version
		cp ../../pd/src/g_all_guis.h ../../externals/build/include
		cp ../../pd/src/g_canvas.h ../../externals/build/include
		cp ../../pd/src/m_imp.h ../../externals/build/include
		cp ../../pd/src/m_pd.h ../../externals/build/include
		cp ../../pd/src/s_stuff.h ../../externals/build/include
		cp ../../pd/src/s_version.h ../../externals/build/include
		cp ../../pd/src/g_all_guis.h ../../externals/build/include
		rm -rf build/
	fi
	if [ $rpi -eq 0 ]
	then
		echo "installing desktop version..."
		test -f debian/control.desktop && cp -f debian/control.desktop debian/control
		test -f ../../l2ork_addons/flext/config-lnx-pd-gcc.txt.intel && cp -f ../../l2ork_addons/flext/config-lnx-pd-gcc.txt.intel ../../externals/grill/trunk/flext/buildsys/config-lnx-pd-gcc.txt
	else
		echo "installing raspbian version..."
		cp -f debian/control.raspbian debian/control
		cp -f ../../l2ork_addons/flext/config-lnx-pd-gcc.txt.rpi ../../externals/grill/trunk/flext/buildsys/config-lnx-pd-gcc.txt
		cat ../../externals/OSCx/src/Makefile | sed -e s/-lpd//g > ../../externals/OSCx/src/Makefile
	fi
	if [[ $os == "win" || $os == "win64" ]]; then
		echo "Making Windows package..."
		echo `pwd`
		make install INCREMENTAL=$INCREMENTAL LIGHT=$LIGHT
		# copy the patch-nwjs.sh script
		cp ../../l2ork_addons/patch-nwjs.sh build/bin
		make package
	elif [[ $os == "osx" ]]; then
		echo "Making OSX package (dmg)..."
		echo `pwd`
		make install INCREMENTAL=$INCREMENTAL LIGHT=$LIGHT
		# copy the patch-nwjs.sh script
		cp ../../l2ork_addons/patch-nwjs.sh build/Purr-Data.app/Contents/Resources/app.nw/bin
		make package
	else
		make install prefix=$inst_dir
		# copy the patch-nwjs.sh script
		cp ../../l2ork_addons/patch-nwjs.sh build$inst_dir/bin
	fi
	echo "copying pd-l2ork-specific externals..."
	# patch_name
	# spectdelay
	if [[ $os == "win" || $os == "win64" ]]; then
		cd ../../l2ork_addons
	elif [[ $os == "osx" ]]; then
		cd ../../l2ork_addons
	elif [ $light -eq 0 ]; then
		cd ../../l2ork_addons/spectdelay/spectdelay~
		./linux-install.sh
		cp -f spectdelay~.pd_linux ../../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
		cp -f spectdelay~-help.pd ../../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
		cp -f array* ../../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
		# return to l2ork_addons folder
		cd ../../
	else
		cd ../../l2ork_addons
	fi
	# install raspberry pi externals (if applicable)
	if [ $inno -eq 0 -a $dmg -eq 0 -a $light -eq 0 ]; then
		cd raspberry_pi
		./makeall.sh
		# these don't seem to be built on some newer ARM architectures
		# for some reason, so check to make sure that we actually have
		# them before trying to install
		if [ -f disis_gpio/disis_gpio.pd_linux ]; then
		cp -f disis_gpio/disis_gpio.pd_linux ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
		cp -f disis_gpio/disis_gpio-help.pd ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
		fi
		if [ -f disis_spi/disis_spi.pd_linux ]; then
		cp -f disis_spi/disis_spi.pd_linux ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
		cp -f disis_spi/disis_spi-help.pd ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
		fi
		cd ../
	fi

	echo "done with l2ork addons."
	cd ../
	# finish install for deb
	if [ $inno -eq 0 -a $dmg -eq 0 ]; then
		cd packages/linux_make
		rm -f build/usr/local/lib/pd
		if [ $pkg -gt 0 ]; then
		echo "tar full installer..."
		if [ $deb -gt 0 ]
		then
			cd build/
			rm -rf DEBIAN/ etc/
			cd ../
			make deb prefix=$inst_dir
		else
			make tarbz2 prefix=$inst_dir
		fi
		echo "move full installer..."
		if [ $deb -gt 0 ]
		then
			mv *.deb ../../
		else
			#rm -f ../../../Pd-l2ork-full-`uname -m`-`date +%Y%m%d`.tar.bz2 2> /dev/null
			#mv build/Pd*bz2 ../../../Pd-l2ork-full-`uname -m`-`date +%Y%m%d`.tar.bz2
			mv -f build/pd*bz2 ../..
		fi
		elif [ $deb -gt 0 ]; then
			make debstage prefix=$inst_dir
			echo "Debian packaging skipped, build results can be found in packages/linux_make/build/."
		fi
		cd ../../
	# move OSX dmg installer
	elif [ $dmg -gt 0 ]; then
		mv packages/darwin_app/Purr*.dmg .
	elif [ $inno -gt 0 ]; then
		if [[ $os == "win64" ]]; then
		    mv packages/win64_inno/Output/Purr*.exe .
		else
		    mv packages/win32_inno/Output/Purr*.exe .
		fi
	fi
fi

cd l2ork_addons/

echo "done."

exit 0
