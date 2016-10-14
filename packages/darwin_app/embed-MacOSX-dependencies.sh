#!/bin/sh
#
# This script finds all of the dependecies from Fink and included them into
# the Pd.app.  <hans@at.or.at>
#
# run it in the root directory where the externals are stored, i.e. "extra"

    
if [ $# -ne 1 ]; then
	echo "Usage: $0 Pd.app-Contents"
	echo "  i.e. $0 /Applications/Pd.app/Contents/"
	exit
fi

LIB_DIR=lib
PD_APP_CONTENTS=$1
PD_APP_LIB=$PD_APP_CONTENTS/$LIB_DIR

#echo "PD_APP_CONTENTS: $PD_APP_CONTENTS"
#echo "PD_APP_LIB: $PD_APP_LIB"

echo " "

for pd_darwin in `find $PD_APP_CONTENTS -name '*.pd_darwin'`; do
	LIBS=`otool -L $pd_darwin | sed -n 's|.*/usr/local/opt/\(.*\.dylib\).*|\1|p'`
	if [ "x$LIBS" != "x" ]; then
		echo "`echo $pd_darwin | sed 's|.*/\(.*\.pd_darwin$\)|\1|'` is using:"
		for lib in $LIBS; do
			echo "    $lib"
			install -d $PD_APP_LIB
			install -p /usr/local/opt/$lib $PD_APP_LIB/$(basename $lib)
			new_lib=`echo $lib | sed 's|.*/\(.*\.dylib\)|\1|'`
			# @executable_path starts from Contents/Resources/app.nw/bin/pd
			install_name_tool -id @executable_path/../../../$LIB_DIR/$new_lib $PD_APP_LIB/$new_lib
			install_name_tool -change /usr/local/opt/$lib @executable_path/../../../$LIB_DIR/$new_lib $pd_darwin
		done
		echo " "
	fi
done

# check for .so plugins used by libquicktime and others
for so in $PD_APP_LIB/*/*.so; do
	LIBS=`otool -L $so | sed -n 's|.*/usr/local/opt/\(.*\.dylib\).*|\1|p'`
	if [ "x$LIBS" != "x" ]; then
		echo "`echo $so | sed 's|.*/\(lib.*/.*\.so\)|\1|'` is using:"
		for lib in $LIBS; do
			echo "    $lib"
			new_lib=`echo $lib | sed 's|.*/\(.*\.dylib\)|\1|'`
			if [ -e  $PD_APP_LIB/$new_lib ]; then
				echo "$PD_APP_LIB/$new_lib already exists, skipping copy."
			else
				install -vp /usr/local/opt/$lib $PD_APP_LIB
			fi
			# @executable_path starts from Contents/Resources/bin/pd
			install_name_tool -change /usr/local/opt/$lib @executable_path/../../../$LIB_DIR/$new_lib $so
		done
		echo " "
	fi
done

for dylib in $PD_APP_LIB/*.dylib; do
	LIBS=`otool -L $dylib | sed -n 's|.*/usr/local/opt/\(.*\.dylib\).*|\1|p'`
	if [ "x$LIBS" != "x" ]; then
		echo "`echo $dylib | sed 's|.*/\(.*\.dylib\)|\1|'` is using:"
		for lib in $LIBS; do
			echo "    $lib"
			new_lib=`echo $lib | sed 's|.*/\(.*\.dylib\)|\1|'`
			if [ -e  $PD_APP_LIB/$new_lib ]; then
				echo "$PD_APP_LIB/$new_lib already exists, skipping copy."
			else
				install -vp /usr/local/opt/$lib $PD_APP_LIB
			fi
			# @executable_path starts from Contents/Resources/bin/pd
			install_name_tool -id @executable_path/../../../$LIB_DIR/$new_lib $PD_APP_LIB/$new_lib
			install_name_tool -change /usr/local/opt/$lib @executable_path/../../../$LIB_DIR/$new_lib $dylib
		done
		echo " "
	fi
done

# run it one more time to catch dylibs that depend on dylibs
for dylib in $PD_APP_LIB/*.dylib; do
	LIBS=`otool -L $dylib | sed -n 's|.*/usr/local/opt/\(.*\.dylib\).*|\1|p'`
	if [ "x$LIBS" != "x" ]; then
		echo "`echo $dylib | sed 's|.*/\(.*\.dylib\)|\1|'` is using:"
		for lib in $LIBS; do
			echo "    $lib"
			new_lib=`echo $lib | sed 's|.*/\(.*\.dylib\)|\1|'`
			if [ -e  $PD_APP_LIB/$new_lib ]; then
				echo "$PD_APP_LIB/$new_lib already exists, skipping copy."
			else
				install -vp /usr/local/opt/$lib $PD_APP_LIB
			fi
			# @executable_path starts from Contents/Resources/app.nw/bin/pd
			install_name_tool -id @executable_path/../../../$LIB_DIR/$new_lib $PD_APP_LIB/$new_lib
			install_name_tool -change /usr/local/opt/$lib @executable_path/../../../$LIB_DIR/$new_lib $dylib
		done
		echo " "
	fi
done

