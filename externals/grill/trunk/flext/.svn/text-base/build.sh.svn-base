#! /bin/bash

# flext - C++ layer for Max/MSP and pd (pure data) externals
#
# Copyright (c) 2001-2005 Thomas Grill (gr@grrrr.org)
# For information on usage and redistribution, and for a DISCLAIMER OF ALL
# WARRANTIES, see the file, "license.txt," in this distribution.  
#
# more information on http://grrrr.org/ext
# ------------------------------------------------------------------------
#
# To build flext or flext-based externals simply run this script.
# Running it without arguments will print some help to the console.
#
# ------------------------------------------------------------------------

flext=${0%/*}/
if [ "$flext" = "$0"/ ]; then flext=./ ; fi

# Arguments:
# $1 - system (pd/max)
# $2 - compiler (msvc/gcc/mingw/cygwin/bcc/icc)
# $3 - target (build/clean/install)

unamesys=$(uname -s)

case $unamesys in
	Linux) platform=lnx;;
	Darwin) platform=mac;;
	CYGWIN*|MINGW*) platform=win;;
	*) echo Platform $unamesys not supported; exit;;
esac

rtsys=$1
compiler=$2
target=$3

# --- The subbatch knows which make utility to use ---
subbatch=${flext}buildsys/build-${compiler}.sh

if 
	[ -n "$platform" -a -n "$rtsys" -a -n "$compiler" -a -f $subbatch ]
then 
	sh $subbatch $platform $rtsys $target $4 $5 $6 $7 $8 $9
else
	echo 
	echo SYNTAX: build.sh [system] [compiler] {target}
	echo system ..... pd / max
	echo compiler ... msvc / gcc / mingw / cygwin / bcc / icc
	echo target ..... build \(default\) / clean / install
	echo 
	echo Please make sure that your make program and compiler can be accessed with the
	echo system path and that all relevant environment variables are properly set.
	echo
	echo For further information read flext/build.txt
	echo
fi
