#! /bin/bash

build=${0%/*}/

make -f ${build}gnumake.mak PLATFORM=$1 RTSYS=$2 COMPILER=cygwin BUILDPATH=${build} $3 $4 $5 $6 $7 $8 $9
