#!/bin/sh
#
# This script finds all of the dependecies from Homebrew or MacPorts and
# resigns them (this is meant to be run after embed-MacOSX-dependencies.sh)
#
# run it in the root directory where the externals are stored, i.e. "extra"

    
if [ $# -ne 1 ]; then
	echo "Usage: $0 Pd.app-Contents"
	echo "  i.e. $0 /Applications/Pd.app/Contents/"
	exit
fi

# Check whether we have Homebrew or MacPorts, prefer the former.
optlocal=$((test -n "$HOMEBREW_PREFIX" && test -d $HOMEBREW_PREFIX/opt && echo $HOMEBREW_PREFIX/opt) || (test -d /opt/local && echo /opt/local) || echo /usr/local)
# Determine the actual installation prefix. On Homebrew, this is
# $HOMEBREW_PREFIX, otherwise (MP or none) it's just $optlocal.
if test -n "$HOMEBREW_PREFIX" && test -d $HOMEBREW_PREFIX; then
    usrlocal=$HOMEBREW_PREFIX
else
    usrlocal=$optlocal
fi

LIB_DIR=lib
PD_APP_CONTENTS=$1
PD_APP_LIB=$PD_APP_CONTENTS/$LIB_DIR
PD_APP_PLUGINS=$PD_APP_CONTENTS/Plugins

#echo "PD_APP_CONTENTS: $PD_APP_CONTENTS"
#echo "PD_APP_LIB: $PD_APP_LIB"

echo " "

for pd_darwin in `find $PD_APP_CONTENTS -name '*.pd_darwin'`; do
    codesign --force -s - $pd_darwin
done

# check for .so plugins used by libquicktime and others
for so in $PD_APP_PLUGINS/*/*.so; do
    codesign --force -s - $so
done

if test -d $PD_APP_LIB; then

for dylib in $PD_APP_LIB/*.dylib; do
    codesign --force -s - $dylib
done

fi