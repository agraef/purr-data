#!/bin/sh
#
# This script finds all of the dependecies from Homebrew or MacPorts and
# includes them into the Pd.app.  <hans@at.or.at>
#
# run it in the root directory where the externals are stored, i.e. "extra"

    
if [ $# -ne 1 ]; then
	echo "Usage: $0 Pd.app-Contents"
	echo "  i.e. $0 /Applications/Pd.app/Contents/"
	exit
fi

# Check whether we have Homebrew or MacPorts, prefer the former.
optlocal=$((test -d /usr/local/opt && echo /usr/local/opt) || (test -d /opt/local && echo /opt/local) || echo /usr/local)

if [ "$optlocal" == "/opt/local" ]; then
    # MacPorts installs software into /opt/local
    usrlocal=$optlocal
else
    # Homebrew links software into /usr/local; if neither MP nor Homebrew is
    # detected, fall back to plain old local software in /usr/local
    usrlocal=/usr/local
fi

LIB_DIR=lib
PD_APP_CONTENTS=$1
PD_APP_LIB=$PD_APP_CONTENTS/$LIB_DIR
PD_APP_PLUGINS=$PD_APP_CONTENTS/Plugins

#echo "PD_APP_CONTENTS: $PD_APP_CONTENTS"
#echo "PD_APP_LIB: $PD_APP_LIB"

echo " "

for pd_darwin in `find $PD_APP_CONTENTS -name '*.pd_darwin'`; do
	LIBS=`otool -L $pd_darwin | sed -n 's|.*'"${optlocal}"'/\(.*\.dylib\).*|\1|p'`
	if [ "x$LIBS" != "x" ]; then
		echo "`echo $pd_darwin | sed 's|.*/\(.*\.pd_darwin$\)|\1|'` is using:"
		for lib in $LIBS; do
			echo "    $lib"
			install -d $PD_APP_LIB
			install -p ${optlocal}/$lib $PD_APP_LIB/$(basename $lib)
			new_lib=`echo $lib | sed 's|.*/\(.*\.dylib\)|\1|'`
			# @executable_path starts from Contents/Resources/app.nw/bin
			install_name_tool -id @executable_path/../../../$LIB_DIR/$new_lib $PD_APP_LIB/$new_lib
			install_name_tool -change ${optlocal}/$lib @executable_path/../../../$LIB_DIR/$new_lib $pd_darwin
		done
		echo " "
	fi
done

# check for libquicktime plugins, copy them over
rm -rf $PD_APP_PLUGINS/libquicktime
libqt_ffmpeg_plugins=${optlocal}/libquicktime-ffmpeg/lib/libquicktime
libqt_plugins=${optlocal}/libquicktime/lib/libquicktime
# prefer libquicktime-ffmpeg, in case we have both installed
if test -d ${libqt_ffmpeg_plugins}; then
    cp -r ${libqt_ffmpeg_plugins} $PD_APP_PLUGINS
elif test -d ${libqt_plugins}; then
    cp -r ${libqt_plugins} $PD_APP_PLUGINS
elif [ "$optlocal" != "/opt/local" ]; then
    echo "No libquicktime found, did you install it?" >&2
fi
# change permissions so that install_name can write to the plugin files
chmod -fR u+xw $PD_APP_PLUGINS/libquicktime

# check for .so plugins used by libquicktime and others
for so in $PD_APP_PLUGINS/*/*.so; do
	LIBS=`otool -L $so | sed -n 's|.*'"${optlocal}"'/\(.*\.dylib\).*|\1|p'`
	if [ "x$LIBS" != "x" ]; then
		echo "`echo $so | sed 's|.*/\(lib.*/.*\.so\)|\1|'` is using:"
		for lib in $LIBS; do
			echo "    $lib"
			new_lib=`echo $lib | sed 's|.*/\(.*\.dylib\)|\1|'`
			if [ -e  $PD_APP_LIB/$new_lib ]; then
				echo "$PD_APP_LIB/$new_lib already exists, skipping copy."
			else
				install -vp ${optlocal}/$lib $PD_APP_LIB
			fi
			# @executable_path starts from Contents/Resources/app.nw/bin
			install_name_tool -change ${optlocal}/$lib @executable_path/../../../$LIB_DIR/$new_lib $so
		done
		echo " "
	fi
	# Homebrew also has some libraries in its cellar (keg-only package?).
	LIBS=`otool -L $so | sed -n 's|.*'"/usr/local/Cellar"'/\(.*\.dylib\).*|\1|p'`
	if [ "x$LIBS" != "x" ]; then
		echo "`echo $so | sed 's|.*/\(lib.*/.*\.so\)|\1|'` is using:"
		for lib in $LIBS; do
			echo "    $lib"
			new_lib=`echo $lib | sed 's|.*/\(.*\.dylib\)|\1|'`
			if [ -e  $PD_APP_LIB/$new_lib ]; then
				echo "$PD_APP_LIB/$new_lib already exists, skipping copy."
			else
				install -vp /usr/local/Cellar/$lib $PD_APP_LIB
			fi
			# @executable_path starts from Contents/Resources/app.nw/bin
			install_name_tool -change /usr/local/Cellar/$lib @executable_path/../../../$LIB_DIR/$new_lib $so
		done
		echo " "
	fi
done

for dylib in $PD_APP_LIB/*.dylib; do
	LIBS=`otool -L $dylib | sed -n 's|.*'"${optlocal}"'/\(.*\.dylib\).*|\1|p'`
	if [ "x$LIBS" != "x" ]; then
		echo "`echo $dylib | sed 's|.*/\(.*\.dylib\)|\1|'` is using:"
		for lib in $LIBS; do
			echo "    $lib"
			new_lib=`echo $lib | sed 's|.*/\(.*\.dylib\)|\1|'`
			if [ -e  $PD_APP_LIB/$new_lib ]; then
				echo "$PD_APP_LIB/$new_lib already exists, skipping copy."
			else
				install -vp ${optlocal}/$lib $PD_APP_LIB
			fi
			# @executable_path starts from Contents/Resources/app.nw/bin
			install_name_tool -id @executable_path/../../../$LIB_DIR/$new_lib $PD_APP_LIB/$new_lib
			install_name_tool -change ${optlocal}/$lib @executable_path/../../../$LIB_DIR/$new_lib $dylib
		done
		echo " "
	fi
done

# run it again to catch dylibs that depend on dylibs located in ${usrlocal}/
for dylib in $PD_APP_LIB/*.dylib; do
	LIBS=`otool -L $dylib | sed -n 's|.*'"${usrlocal}"'/\(.*\.dylib\).*|\1|p'`
	if [ "x$LIBS" != "x" ]; then
		echo "`echo $dylib | sed 's|.*/\(.*\.dylib\)|\1|'` is using:"
		for lib in $LIBS; do
			echo "    $lib"
			new_lib=`echo $lib | sed 's|.*/\(.*\.dylib\)|\1|'`
			if [ -e  $PD_APP_LIB/$new_lib ]; then
				echo "$PD_APP_LIB/$new_lib already exists, skipping copy."
			else
				install -vp ${usrlocal}/$lib $PD_APP_LIB
			fi
			# @executable_path starts from Contents/Resources/app.nw/bin
			install_name_tool -id @executable_path/../../../$LIB_DIR/$new_lib $PD_APP_LIB/$new_lib
			install_name_tool -change ${usrlocal}/$lib @executable_path/../../../$LIB_DIR/$new_lib $dylib
		done
		echo " "
	fi
done

# finally, run it one more time to catch dylibs that depend on dylibs from
# ${usrlocal}/
for dylib in $PD_APP_LIB/*.dylib; do
        LIBS=`otool -L $dylib | sed -n 's|.*'"${usrlocal}"'/\(.*\.dylib\).*|\1|p'`
        if [ "x$LIBS" != "x" ]; then
                echo "`echo $dylib | sed 's|.*/\(.*\.dylib\)|\1|'` is using:"
                for lib in $LIBS; do
                        echo "    $lib"
                        new_lib=`echo $lib | sed 's|.*/\(.*\.dylib\)|\1|'`
                        if [ -e  $PD_APP_LIB/$new_lib ]; then
                                echo "$PD_APP_LIB/$new_lib already exists, skipping copy."
                        else
                                install -vp ${usrlocal}/$lib $PD_APP_LIB
                        fi
                        # @executable_path starts from Contents/Resources/app.nw/bin
                        install_name_tool -id @executable_path/../../../$LIB_DIR/$new_lib $PD_APP_LIB/$new_lib
                        install_name_tool -change ${usrlocal}/$lib @executable_path/../../../$LIB_DIR/$new_lib $dylib
                done
                echo " "
        fi
done
