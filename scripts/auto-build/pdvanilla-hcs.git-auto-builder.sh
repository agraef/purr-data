#!/bin/sh

# the source dir where this script is
## this could be done more easily with ${0%/*}
SCRIPT_DIR=$(echo $0 | sed 's|\(.*\)/.*$|\1|')
. $SCRIPT_DIR/auto-build-common

# the name of this script
## this could be done more easily with ${0##*/}
SCRIPT=$(echo $0| sed 's|.*/\(.*\)|\1|g')

# convert into absolute path
cd "${SCRIPT_DIR}/../.."
auto_build_root_dir=`pwd`
echo "build root: $auto_build_root_dir" 
rsync_distro "$auto_build_root_dir"


cd "${auto_build_root_dir}"
echo "--------------------------------------------------------------------------------"
git pull
echo "--------------------------------------------------------------------------------"
./autogen.sh
echo "--------------------------------------------------------------------------------"
./configure
echo "--------------------------------------------------------------------------------"
make
echo "--------------------------------------------------------------------------------"
mkdir "${auto_build_root_dir}/testinstall"
make DESTDIR="${auto_build_root_dir}/testinstall" install
echo "--------------------------------------------------------------------------------"
make dist


# since the above test can cause this script to exit with an error, force it
# to be happy to prevent getting automated error emails to root
true 
