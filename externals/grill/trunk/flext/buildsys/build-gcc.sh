#! /bin/bash

build=${0%/*}/

if [ $(which make) ]; then
	MAKE=make
elif [ $(which mingw32-make) ]; then
	MAKE=mingw32-make
else
	echo make utility not found
	exit
fi

$MAKE -f ${build}gnumake.mak PLATFORM=$1 RTSYS=$2 COMPILER=gcc BUILDPATH=${build} $3 $4 $5 $6 $7 $8 $9
