#!/bin/bash
# build the 3 msd objects using flext and its configuration


# build
cd ../msd
../../../grill/flext/build.sh pd gcc
cd ../msd2D
../../../grill/flext/build.sh pd gcc	
cd ../msd3D
../../../grill/flext/build.sh pd gcc
cd ../src

# move objects
UNAME=`uname -s`
if [ $UNAME = "Darwin" ]
then
	mv ../msd/pd-darwin/release-single/msd.pd_darwin ../msd/msd.pd_darwin
	mv ../msd2D/pd-darwin/release-single/msd2D.pd_darwin ../msd2D/msd2D.pd_darwin
	mv ../msd3D/pd-darwin/release-single/msd3D.pd_darwin ../msd3D/msd3D.pd_darwin
fi

if [ $UNAME = "Linux" ]
then
    cd ../msd
    ../../../grill/flext/build.sh pd gcc install
    cd ../msd2D
    ../../../grill/flext/build.sh pd gcc install
    cd ../msd3D
    ../../../grill/flext/build.sh pd gcc install
    cd ../src
fi
