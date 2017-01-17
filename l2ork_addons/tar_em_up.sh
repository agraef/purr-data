#!/bin/bash
# super-simplistic installer for l2ork things by Ivica Ico Bukvic <ico@vt.edu>
# for info on L2Ork visit http://l2ork.music.vt.edu

if [ $# -eq 0 ] # should check for no arguments
then
	echo
	echo "   Usage: ./tar_em_up.sh -option1 -option2 ..."
	echo "   Options:"
	echo "     -a    l2ork addon to the dev package"
	echo "     -b    build a deb (incremental, all platforms)"
	echo "     -B    build a deb (complete recompile)"
	echo "     -c    core Pd source tarball"
	echo "     -e    everything"
	echo "     -f    full installer (incremental)"
	echo "     -F    full installer (complete recompile)"
	echo "     -n    skip package creation (-bB, -fF)"
	echo "     -R    build a Raspberry Pi deb (complete recompile)"
	echo "     -r    build a Raspberry Pi deb (incremental)"
	echo "     -w    install custom version of cwiid system-wide"
	echo "     -X    build an OSX installer (dmg)"
	echo "     -z    build a Windows installer (incremental)"
	echo "     -Z    build a Windows installer (complete recompile)"
	echo
	echo "   For custom install locations do the following before"
	echo "   running this script:"
	echo
	echo "           export inst_dir=/some/custom/location"
	echo
	exit 1
fi

addon=0
deb=0
core=0
full=0
sys_cwiid=0
rpi=0
pkg=1
inno=0
dmg=0

# Get the OS we're running under, normalized to names that can be used
# to fetch the nwjs binaries below

os=`uname | tr '[:upper:]' '[:lower:]'`
if [[ $os == *"mingw32"* ]]; then
	os=win
fi
if [[ $os == "darwin" ]]; then
	os=osx
fi

# Fetch the nw.js binary if we haven't already. We want to fetch it even
# for building with no libs, so we do it before all options
echo nwjs-sdk-v0.16.0-`uname | tr '[:upper:]' '[:lower:]'`
if [ ! -d "../pd/nw/nw" ]; then
	if [ `getconf LONG_BIT` -eq 32 ]; then
		arch="ia32"
	else
		arch="x64"
	fi

	# for rpi
	if [ `uname -m` == "armv7l" ]; then
		arch="armv7l"
	fi

	if [[ $os == "win" || $os == "osx" ]]; then
		ext="zip"
	else
		ext="tar.gz"
	fi

	if [[ $os == "win" ]]; then
		# We need the lts version to be able to run on XP. For
                # simplicity we use that same version for 64 bit Windows, too
		nwjs_version="v0.14.7"
	else
		# temporary kluge for rpi-- only 0.15.1 is available atm
		if [ `uname -m` == "armv7l" ]; then
			nwjs_version="v0.15.1"
		else
			nwjs_version="v0.18.4"
		fi
	fi

	nwjs="nwjs-sdk"
	nwjs_dirname=${nwjs}-${nwjs_version}-${os}-${arch}
	nwjs_filename=${nwjs_dirname}.${ext}
	nwjs_url=https://git.purrdata.net/jwilkes/nwjs-binaries/raw/master
	nwjs_url=${nwjs_url}/$nwjs_filename
	echo "Fetching the nwjs binary from"
	echo "$nwjs_url"
	wget $nwjs_url
	if [[ $os == "win" ]]; then
		unzip $nwjs_filename
	else
		tar -xf $nwjs_filename
	fi
	# Special case for arm binary's inconsistent directory name
	# (It's not the same as the `uname -m` output)
	if [ `uname -m` == "armv7l" ]; then
		nwjs_dirname=`echo $nwjs_dirname | sed 's/armv7l/arm/'`
	fi
        mv $nwjs_dirname ../pd/nw/nw
	# make sure the nw binary is executable on GNU/Linux
	if [[ $os != "win" && $dmg == 0 ]]; then
		chmod 755 ../pd/nw/nw/nw
	fi
	rm $nwjs_filename
fi

# For Windows, fetch the ASIO SDK if we don't have it already
if [[ $os == "win" ]]; then
	if [ ! -d "../pd/lib" ]; then
		mkdir ../pd/lib
		wget http://www.steinberg.net/sdk_downloads/asiosdk2.3.zip
		unzip asiosdk2.3.zip
		mv ASIOSDK2.3 ../pd/lib
	fi
fi


while getopts ":abBcdefFnRruwXzZ" Option
do case $Option in
		a)		addon=1;;

		b)		deb=1
				inst_dir=${inst_dir:-/usr};;

		B)		deb=2
				inst_dir=${inst_dir:-/usr};;

		c)		core=1;;

		e)		addon=1
				core=1
				full=1;;

		f)		full=1;;

		F)		full=2;;

		n)		pkg=0;;

		R)		deb=2
				inst_dir=/usr
				rpi=1;;

		r)		deb=1
				inst_dir=/usr
				rpi=1;;

		w)		sys_cwiid=1
				;;

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

export TAR_EM_UP_PREFIX=$inst_dir

cd ../

if [ $core -eq 1 ]
then
	echo "core Pd..."
	rm -f ../Pd-l2ork-`date +%Y%m%d`.tar.bz2 2> /dev/null
	cd pd/src/
	make clean
	cd ../../
	tar -jcf ./Pd-l2ork-`date +%Y%m%d`.tar.bz2 pd
fi

if [ $full -gt 0 -o $deb -gt 0 -o $inno -gt 0 -o $dmg -gt 0 ]
then
	echo "Pd-L2Ork full installer... IMPORTANT! To ensure you have the most up-to-date submodules, this process requires internet connection to pull sources from various repositories..."

	if [ -d .git ]; then
		# check if Gem submodule is empty, and if so do first init
		if [ "$(ls -A Gem)" ]; then
			git submodule update
			#git submodule foreach git pull origin master
		else
			# init all submodules (only necessary the first time)
			git submodule init
			git submodule update
			#git submodule foreach git pull origin master
		fi
	fi


	if [ $full -eq 2 -o $deb -eq 2 -o $inno -eq 2 -o $dmg -eq 2 ]
	then
	#	echo "Since we are doing a complete recompile we are assuming we will need to install l2ork version of the cwiid library. You will need to remove any existing cwiid libraries manually as they will clash with this one. L2Ork version is fully backwards compatible while also offering unique features like full extension support including the passthrough mode. YOU SHOULD REMOVE EXISTING CWIID LIBRARIES PRIOR TO RUNNING THIS INSTALL... You will also have to enter sudo password to install these... Press any key to continue or CTRL+C to cancel install..."
	#	read dummy
		# clean files that may remain stuck even after doing global make clean (if any)
		cd externals/miXed
		make clean
		cd ../
		make gem_clean
		cd ../Gem/src/
		make distclean
		rm -rf ./.libs
		rm -rf ./*/.libs
		cd ../
		make distclean
		rm gemglutwindow.pd_linux
		rm Gem.pd_linux
		aclocal
		./autogen.sh
		export INCREMENTAL=""
	else
		cd Gem/
		export INCREMENTAL="yes"
	fi
	cd ../pd/src && aclocal && autoconf
	if [[ $os == "win" ]]; then
		cd ../../packages/win32_inno
	elif [[ $os == "osx" ]]; then
		cd ../../packages/darwin_app
	else
		cd ../../packages/linux_make
	fi
	if [ $full -gt 1 -o $deb -eq 2 -o $inno -eq 2 -o $dmg -eq 2 ]
	then
		make distclean
		cp ../../pd/src/g_all_guis.h ../../externals/build/include
		cp ../../pd/src/g_canvas.h ../../externals/build/include
		cp ../../pd/src/m_imp.h ../../externals/build/include
		cp ../../pd/src/m_pd.h ../../externals/build/include
		cp ../../pd/src/s_stuff.h ../../externals/build/include
		cp ../../pd/src/t_tk.h ../../externals/build/include
		cp ../../pd/src/g_all_guis.h ../../externals/build/include								
		rm -rf build/
	fi
	if [ $rpi -eq 0 ]
	then
		echo "installing desktop version..."
		cp -f debian/control.desktop debian/control
		cp -f ../../l2ork_addons/flext/config-lnx-pd-gcc.txt.intel ../../externals/grill/trunk/flext/buildsys/config-lnx-pd-gcc.txt
	else
		echo "installing raspbian version..."
		cp -f debian/control.raspbian debian/control
		cp -f ../../l2ork_addons/flext/config-lnx-pd-gcc.txt.rpi ../../externals/grill/trunk/flext/buildsys/config-lnx-pd-gcc.txt
		cat ../../externals/OSCx/src/Makefile | sed -e s/-lpd//g > ../../externals/OSCx/src/Makefile
	fi
	if [[ $os == "win" ]]; then
		echo "Making Windows package..."
		echo `pwd`
		make install && make package
	elif [[ $os == "osx" ]]; then
		echo "Making OSX package (dmg)..."
		echo `pwd`
		make install && make package
	else
		make install prefix=$inst_dir
	fi
	echo "copying pd-l2ork-specific externals..."
	# create images folder
	mkdir -p ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra/images
	# patch_name
	# spectdelay
	if [[ $os == "win" ]]; then
		cd ../../l2ork_addons
	elif [[ $os == "osx" ]]; then
		cd ../../l2ork_addons
	else
		cd ../../l2ork_addons/spectdelay/spectdelay~
		./linux-install.sh
		cp -f spectdelay~.pd_linux ../../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
		cp -f spectdelay~-help.pd ../../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
		cp -f array* ../../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
		# return to l2ork_addons folder
		cd ../../
	fi
	# install raspberry pi externals (if applicable)
	if [ $inno -eq 0 -a $dmg -eq 0 ]; then
		cd raspberry_pi
		./makeall.sh
		cp -f disis_gpio/disis_gpio.pd_linux ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
		cp -f disis_gpio/disis_gpio-help.pd ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
		cp -f disis_spi/disis_spi.pd_linux ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
		cp -f disis_spi/disis_spi-help.pd ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
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
		fi
		cd ../../
	# move OSX dmg installer
	elif [ $dmg -gt 0 ]; then
		mv packages/darwin_app/Pd*.dmg .
	fi
fi

if [ $addon -eq 1 ]
then
	echo "l2ork addons..."
	rm -f ../l2ork_addons-`uname -m`-`date +%Y%m%d`.tar.bz2 2> /dev/null
	#cp -rf /usr/local/lib/pd/* l2ork_addons/externals/
	tar -jcf ../l2ork_addons-`uname -m`-`date +%Y%m%d`.tar.bz2 l2ork_addons
fi

cd l2ork_addons/

echo "done."

exit 0
