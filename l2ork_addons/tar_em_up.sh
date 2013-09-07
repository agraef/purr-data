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
	echo "     -c    core Pd"
	echo "     -d    whole dev folder"
	echo "     -e    everything"
	echo "     -f    full installer (incremental)"
	echo "     -F    full installer (complete recompile)"
	echo "     -R    build a Raspberry Pi deb (complete recompile)"
	echo "     -r    build a Raspberry Pi deb (incremental)"
	echo "     -w    install custom version of cwiid system-wide"
	echo
	exit 1
fi

addon=0
deb=0
core=0
dev=0
full=0
sys_cwiid=0
rpi=0

inst_dir=${inst_dir:-/usr/local}

while getopts ":abBcdefFRruw" Option
do case $Option in
		a)		addon=1;;

		b)		deb=1
				inst_dir=/usr;;

		B)		deb=2
				inst_dir=/usr;;

		c)		core=1;;

		d)		dev=1;;

		e)		addon=1
				core=1
				dev=1
				full=1;;

		f)		full=1;;

		F)		full=2;;

		R)		deb=2
				inst_dir=/usr
				rpi=1;;

		r)		deb=1
				inst_dir=/usr
				rpi=1;;

		w)		sys_cwiid=1
				;;

		*)		echo "Error: unknown option";;
	esac
done

cd ../

if [ $core -eq 1 ]
then
	echo "core Pd..."
	rm -f ../Pd-l2ork-`date +%Y%m%d`.tar.bz2 2> /dev/null
	cd pd/src/
	make clean
	cd ../../
	tar -jcf ../Pd-l2ork-`date +%Y%m%d`.tar.bz2 pd
fi

if [ $dev -eq 1 ]
then
	echo "Pd dev package..."
	#cd doc/
	#svn checkout https://pure-data.svn.sourceforge.net/svnroot/pure-data/trunk/doc .
	#cp -f ../l2ork_addons/doc/Makefile .
	#cd ..
	cd externals/miXed
	make clean
	cd ../
	make distclean
	cd ../pd/src
	make distclean
	cd ../../Gem/src/
	make distclean
	rm -rf ./.libs
	rm -rf ./*/.libs
	cd ../
	make distclean
	rm gemglutwindow.pd_linux
	rm Gem.pd_linux
	cd ../packages/linux_make
	make distclean
	cd ../../
	gitfolder=`basename $PWD`
	cd ../
	rm -f pd-l2ork-dev-`date +%Y%m%d`.tar.bz2 2> /dev/null
	echo "tar dev installer..."
	tar -jcf pd-l2ork-dev-`date +%Y%m%d`.tar.bz2 $gitfolder
	cd $gitfolder
fi

if [ $full -gt 0 -o $deb -gt 0 ]
then
	echo "Pd full installer... IMPORTANT! When ran for the first time this step requires internet connection to pull sources from other repositories..."

	# check if Gem submodule is empty, and if so do first init
	if [ "$(ls -A Gem)" ]; then
		git submodule update
	else
		git submodule init
		git submodule update
	fi

	# update the include files to be safe
	#if [ ! -d "/usr/local/include/pdl2ork" ]; then
	#	sudo mkdir /usr/local/include/pdl2ork
	#fi

	#if [ $full -eq 3 ]
	#then
	#	echo "IMPORTANT! If you are already running vanilla Pd or Pd-extended, or have a custom installation of Pd-l2ork at a location other than /usr/local/lib/pd-l2ork, you will want to EITHER uninstall all older versions of Pd-l2ork and/or other versions of pd OR manually pre-install Pd-l2ork includes in order to ensure that any third-party externals that rely on the global Pd-l2ork includes reference the right versions of the said files. Failing to do so may result in incorrectly compiled externals that will definitely crash Pd-l2ork. You can install the includes into their default dir /usr/local/include by typing following commands:"
	#	echo
	#	echo "sudo cp -f -v pd/src/g_all_guis.h /usr/local/include/pdl2ork/"
	#	echo "sudo cp -f -v pd/src/g_canvas.h /usr/local/include/pdl2ork/"
	#	echo "sudo cp -f -v pd/src/m_imp.h /usr/local/include/pdl2ork/"
	#	echo "sudo cp -f -v pd/src/m_pd.h /usr/local/include/pdl2ork/"
	#	echo "sudo cp -f -v pd/src/s_stuff.h /usr/local/include/pdl2ork/"
	#	echo
	#	echo "If you don't have sudo enabled, replace sudo commands with the appropraite alternative before pasting aforesaid lines in the terminal.
#
#PLEASE NOTE that because both Pd and Pd-l2ork use the includes with the same name, depending on your system's setup, skipping this step may result in a failed build. Press any key to continue or CTRL+C to cancel install and manually copy the said files first (or use -f or -F options to have these steps performed automatically)..."
	#	read dummy
	#else
	#	echo "First we will copy updated includes... You may have to enter your sudo password..."
	#	sudo mkdir $inst_dir/include/pdl2ork/
	#	sudo cp -f -v pd/src/g_all_guis.h $inst_dir/include/pdl2ork/
	#	sudo cp -f -v pd/src/g_canvas.h $inst_dir/include/pdl2ork/
	#	sudo cp -f -v pd/src/m_imp.h $inst_dir/include/pdl2ork/
	#	sudo cp -f -v pd/src/m_pd.h $inst_dir/include/pdl2ork/
	#	sudo cp -f -v pd/src/s_stuff.h $inst_dir/include/pdl2ork/
	#fi

	# update docs
	#cd doc/
	#svn checkout https://pure-data.svn.sourceforge.net/svnroot/pure-data/trunk/doc .
	#cp -f ../l2ork_addons/doc/Makefile .
	#cd ..

	if [ $full -eq 2 -o $deb -eq 2 ]
	then
	#	echo "Since we are doing a complete recompile we are assuming we will need to install l2ork version of the cwiid library. You will need to remove any existing cwiid libraries manually as they will clash with this one. L2Ork version is fully backwards compatible while also offering unique features like full extension support including the passthrough mode. YOU SHOULD REMOVE EXISTING CWIID LIBRARIES PRIOR TO RUNNING THIS INSTALL... You will also have to enter sudo password to install these... Press any key to continue or CTRL+C to cancel install..."
	#	read dummy
	#	if [ $no_cwiid -eq 0 ]
	#	then
			cd l2ork_addons/cwiid/
			# install cwiid
			aclocal
			autoconf
			./configure --with-python=python2
			make
			# we have disabled system-wide install because as of 23-03-2013
			# we now statically link disis_wiimote against custom L2Ork version
			# of the cwiid library
			if [ $sys_cwiid -eq 1 ]
			then
				sudo make install
			fi
			cd ../../
	#	fi
		# clean files that may remain stuck even after doing global make clean (if any)
		cd externals/miXed
		make clean
		cd ../../Gem/src/
		make distclean
		rm -rf ./.libs
		rm -rf ./*/.libs
		cd ../
		make distclean
		rm gemglutwindow.pd_linux
		rm Gem.pd_linux
		aclocal
		./autogen.sh
	#elif [ $full -eq 3 ]
	#then
	#	echo "Since pd-l2ork relies on a unique version of cwiid library, we will need to install it to make disis_wiimote external work properly. YOU SHOULD REMOVE EXISTING CWIID LIBRARIES PRIOR TO RUNNING THIS INSTALL... No worries though, L2Ork version is fully backwards compatible while also offering unique features like full extension support including the passthrough mode. To install cwiid library go to <pd-l2ork-root-git-folder>/l2ork-addons/cwiid/ folder and install it using the usual:"
	#	echo
	#	echo "./configure"
	#	echo "make"
	#	echo "sudo make install"
	#	echo
	#	echo "As an alternative, you can also use the -f or -F options instead of an -u option to have this performed automatically. Please note that options -f and -F require that your system has sudo enabled. Press any key to continue or CTRL+C to cancel install..."
	#	read dummy
		# clean files that may remain stuck even after doing global make clean (if any)
	#	cd externals/miXed
	#	make clean
	#	cd ../../Gem/src/
	#	make distclean
	#	rm -rf ./.libs
	#	rm -rf ./*/.libs
	#	cd ../
	#	make distclean
	#	rm gemglutwindow.pd_linux
	#	rm Gem.pd_linux
	#	aclocal
	#	./autogen.sh
	else
		cd Gem/
	fi
	cd ../packages/linux_make
	if [ $full -gt 1 -o $deb -eq 2 ]
	then
		make distclean
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
	make install prefix=$inst_dir
	echo "copying l2ork-specific externals..."
	# patch_name
	cd ../../l2ork_addons/patch_name
	make clean
	make
	cp -f patch_name.pd_linux ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
	cp -f patch_name-help.pd ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
	# disis_wiimote
	cd ../disis_wiimote
	make clean
	make
	cp -f disis_wiimote.pd_linux ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
	cp -f disis_wiimote-help.pd ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
	# disis_netsend
	cd ../disis_netsend
	make clean
	make
	cp -f disis_netsend.pd_linux ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
	cp -f disis_netsend-help.pd ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
	# disis_netreceive
	cd ../disis_netreceive
	make clean
	make
	cp -f disis_netreceive.pd_linux ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
	cp -f disis_netreceive-help.pd ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
	# disis_phasor
	cd ../disis_phasor
	make clean
	make
	cp -f disis_phasor~.pd_linux ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
	cp -f disis_phasor~-help.pd ../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
	# spectdelay
	cd ../spectdelay/spectdelay~
	./linux-install.sh
	cp -f spectdelay~.pd_linux ../../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
	cp -f spectdelay~-help.pd ../../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
	cp -f array* ../../../packages/linux_make/build$inst_dir/lib/pd-l2ork/extra
	# return to l2ork_addons folder
	cd ../../
	# finish install
	cd ../packages/linux_make
	echo "tar full installer..."
	rm -f build/usr/local/lib/pd
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
		mv *.deb ../../../
	else
		#rm -f ../../../Pd-l2ork-full-`uname -m`-`date +%Y%m%d`.tar.bz2 2> /dev/null
		#mv build/Pd*bz2 ../../../Pd-l2ork-full-`uname -m`-`date +%Y%m%d`.tar.bz2
		mv -f build/pd*bz2 ../../..
	fi
	cd ../../
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
