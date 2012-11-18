#!/bin/sh
# this script is the first attempt to have an automated updater and builder

# the source dir where this script is
## this could be done more easily with ${0%/*}
SCRIPT_DIR=$(echo $0 | sed 's|\(.*\)/.*$|\1|')
. $SCRIPT_DIR/auto-build-common

# the name of this script
## this could be done more easily with ${0##*/}
SCRIPT=$(echo $0| sed 's|.*/\(.*\)|\1|g')

BUILD_DIR=.
case $SYSTEM in 
	linux)
		BUILD_DIR=linux_make
		echo "Configuring to use $BUILD_DIR on GNU/Linux"
		;;
	darwin)
		BUILD_DIR=darwin_app
		echo "Configuring to use $BUILD_DIR on Darwin/Mac OS X"
		;;
	mingw*)
		BUILD_DIR=win32_inno
		echo "Configuring to use $BUILD_DIR on MinGW/Windows"
		;;
	cygwin*)
		BUILD_DIR=win32_inno
		echo "Configuring to use $BUILD_DIR on Cygwin/Windows"
		;;
	*)
		echo "ERROR: Platform $SYSTEM not supported!"
		exit
		;;
esac


# convert into absolute path
cd "${SCRIPT_DIR}/../.."
auto_build_root_dir=`pwd`
echo "build root: $auto_build_root_dir" 
rsync_distro "$auto_build_root_dir"

cd "${auto_build_root_dir}/packages/$BUILD_DIR"
make -C "${auto_build_root_dir}/packages" set_version
make test_locations
echo "mounts ----------------------------------------"
mount
print_ip_address
make package_clean
make install && make package


upload_build ()
{
    platform_folder=$1
    build_folder=$2
    archive_format=$3
	
	archive=$(ls -1 ${auto_build_root_dir}/packages/${platform_folder}/${build_folder}/Pd*.${archive_format} | tail -1)
    
    echo "upload specs $1 $2 $3"
    echo "Uploading $archive"
	upload_filename=$(ls -1 ${archive} | sed "s|.*/\(.*\)\.${archive_format}|\1-${HOSTNAME}.${archive_format}|")
	if [ -e ${archive} ]; then
		case $SYSTEM in 
			mingw*)
				/c/cygwin/bin/sh -c \
					"rsync --archive --no-links --copy-links --chmod=a+r ${archive} rsync://128.238.56.50/upload/${DATE}/${upload_filename}" && \
					md5sum ${archive} > ${archive}.md5 && \
					/c/cygwin/bin/sh -c \
					"rsync --archive --no-links --copy-links ${archive}.md5 rsync://128.238.56.50/upload/${DATE}/${upload_filename}.md5" && \
					echo "successfully uploaded: ${upload_filename}" && \
					echo SUCCESS
				;;
			*)
				rsync -a --chmod=a+r ${archive} rsync://128.238.56.50/upload/${DATE}/${upload_filename}  && \
					md5sum ${archive} > ${archive}.md5 && \
					rsync -a ${archive}.md5 rsync://128.238.56.50/upload/${DATE}/${upload_filename}.md5  && \
					echo "successfully uploaded: ${upload_filename}" && \
					echo SUCCESS
				;;
		esac
	fi
}


case $SYSTEM in 
	linux)
		if [ -x /usr/bin/dpkg-deb ]; then
			upload_build linux_make . deb
		else
			upload_build linux_make build tar.bz2
		fi
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

