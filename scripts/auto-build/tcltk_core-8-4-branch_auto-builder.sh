#!/bin/sh
# this script is the first attempt to have an automated updater and builder

# the source dir where this script is
SCRIPT_DIR=$(echo $0 | sed 's|\(.*\)/.*$|\1|')
. $SCRIPT_DIR/auto-build-common

# the name of this script
SCRIPT=$(echo $0| sed 's|.*/\(.*\)|\1|g')


# convert into absolute path
cd $(echo $0 | sed 's|\(.*\)/.*$|\1|')
auto_build_root_dir=$(pwd)
echo "root: $auto_build_root_dir" 

# let rsync handle the cleanup with --delete
rsync -a --delete rsync://128.238.56.50/distros/tcltk_core-8-4-branch/ \
	 ${auto_build_root_dir}/

BUILD_DIR=.
case $SYSTEM in 
	 linux)
		  BUILD_DIR=unix
		  echo "Configuring to use $BUILD_DIR on GNU/Linux"
		  ;;
	 darwin)
		  BUILD_DIR=macosx
		  echo "Configuring to use $BUILD_DIR on Darwin/Mac OS X"
		  export PATH=/bin:/sbin:/usr/bin:/usr/sbin
#		  export CFLAGS="-arch ppc -arch ppc64 -arch i386 -arch x86_64 \
#           -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4"
		  make -C tcl/${BUILD_DIR} deploy
		  make -C tk/${BUILD_DIR} deploy
		  ;;
	 mingw*)
		  BUILD_DIR=win
		  echo "Configuring to use $BUILD_DIR on MinGW/Windows"
		  ;;
	 cygwin*)
		  BUILD_DIR=win
		  echo "Configuring to use $BUILD_DIR on Cygwin/Windows"
		  ;;
	 *)
		  echo "ERROR: Platform $SYSTEM not supported!"
		  exit
		  ;;
esac


# not used (yet...)
exit 



upload_build ()
{
    platform_folder=$1
    build_folder=$2
    archive_format=$3

	 archive="${auto_build_root_dir}/packages/${platform_folder}/${build_folder}/Pd*.${archive_format}"
    
    echo "upload specs $1 $2 $3"
    echo "Uploading $archive"
	 upload_filename=$(ls -1 ${archive} | sed "s|.*/\(.*\)\.${archive_format}|\1-${HOSTNAME}.${archive_format}|")
	 case $SYSTEM in 
		  mingw*)
				test -e ${archive} && /c/cygwin/bin/sh -c \
					 "rsync --archive --no-links --copy-links ${archive} rsync://128.238.56.50/upload/${DATE}/${upload_filename}" &&\
                                         echo "successfully uploaded: ${upload_filename}" && \
					 echo SUCCESS
				;;
		  *)
				test -e ${archive} && rsync -a ${archive} \
					 rsync://128.238.56.50/upload/${DATE}/${upload_filename}  && \
                                         echo "successfully uploaded: ${upload_filename}" && \
					 echo SUCCESS
				;;
		  esac
}


case $SYSTEM in 
	 linux)
		  upload_build linux_make build tar.bz2
		  ;;
	 darwin)
		  upload_build darwin_app . dmg
		  ;;
	 mingw*)
		  upload_build win32_inno Output exe
		  ;;
	 cygwin*)
		  upload_build win32_inno Output exe
		  ;;
esac

