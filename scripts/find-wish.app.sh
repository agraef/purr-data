#!/bin/sh

cvs_root_dir=`echo $0 | sed 's|\(.*\)/.*$|\1|'`/..
cd $cvs_root_dir
cvs_root_dir=`pwd`

TCLTK="$1"

if [ "x${TCLTK}" != "x" ]; then
	 test -d /Volumes/${TCLTK} || \
		  hdiutil mount -quiet ${cvs_root_dir}/packages/darwin_app/${TCLTK}.dmg
	 echo "/Volumes/${TCLTK}"
else
	 if [ -d "/Library/Frameworks/Tk.framework/Resources" ]; then
		  echo "/Library/Frameworks/Tk.framework/Resources"
	 else 
		  if [ -d "/System/Library/Frameworks/Tk.framework/Resources" ]; then
				echo "/System/Library/Frameworks/Tk.framework/Resources"
		  fi
	 fi
fi

