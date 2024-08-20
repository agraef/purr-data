#! /bin/bash

# Unfortunately, at present there doesn't seem to be a version of nw.js where
# everything just works fine on all platforms. Thus you may want or need to
# patch up your existing installation with a different nw.js version. Or maybe
# you just want to experiment with different versions and see what works best
# on your system. I wrote this script to automate this process, since it is
# quite error-prone and arduous when done manually.

# The script can be downloaded at:

# https://github.com/agraef/purr-data/blob/testing/l2ork_addons/patch-nwjs.sh

# Put it somewhere where you can create files, make sure that it's executable,
# and run it, e.g., as: ./patch-nwjs.sh 0.42.3

# The script will then download the given nw.js version and replace the
# version that was included in your current purr-data installation with it. It
# also takes care of removing your existing purr-data-specific nw.js config,
# which might otherwise cause issues when downgrading the nw.js version
# (which is the main use case, since our packages currently ship with pretty
# much the last nw.js version that is known to work properly with purr-data).

# You can also invoke the script without the nw.js version argument, it will
# then use the default version that our packages ship with (0.67.0 at the time
# of this writing). Or run ./patch-nwjs.sh -h to print some usage information.

# The script also lets you edit the package.json file of the installed
# application to quickly switch NW2 mode on or off. Note that NW2 mode
# (https://nwjs.io/blog/nw2-mode/) is required for Purr Data's window
# management to work properly with any nw.js version after 0.67.1. But it
# slows down JavaScript execution quite a lot, so that Purr Data's user
# interface may become slow and unresponsive. Thus enabling this options isn't
# recommended, and the latest packages we ship have it disabled. (If you're
# still running an older Purr Data release and experience these slow-downs,
# you can run ./patch-nwjs.sh -w1 to fix this.)

# To use this operation, you invoke the script with the -nw1 or -nw2 option,
# indicating whether NW2 mode should be disabled or not. The -nw1 or -nw2
# option can also be combined with the nw.js version argument if you want to
# change both the version and the NW2 switch at the same time. Note that using
# the -nw1 option with nw.js versions prior to 0.42.4 is possible but
# unnecessary, since older versions have the NW2 option disabled by default.

# To perform its operations, the script requires administrator access so that
# it is able to modify the installed application. On Linux and Mac, the script
# uses sudo for this purpose. On Windows, the script employs the gsudo program
# (https://gerardog.github.io/gsudo/), which is a sudo replacement for Windows
# systems which works nicely with msys2 bash. If you don't have this installed
# then the automatic installation will fail and you will have to install
# things manually.

nwjs_version=""
nwopt=""

case "$1" in
    -nw1) nwopt=nw1; shift;;
    -nw2) nwopt=nw2; shift;;
    [0-9]*) nwjs_version="$1"; shift;;
esac

case "$1" in
    -nw1) nwopt=nw1;;
    -nw2) nwopt=nw2;;
    [0-9]*) nwjs_version="$1" ;;
    "") ;;
    *) echo "USAGE: $0 [-h] [-nw1|-nw2] [nwjs-version]" 1>&2
       echo "-h: print this message" 1>&2
       echo "-nw1: switch NW2 mode OFF (https://nwjs.io/blog/nw2-mode/)" 1>&2
       echo "-nw2: switch NW2 mode ON (this may slow down the application!)" 1>&2
       echo "nwjs-version (x.y.z): nw.js version to be used (default: 0.67.0)" 1>&2
       exit 0 ;;
esac

if [ -z "$nwopt" -a -z "$nwjs_version" ]; then
    nwjs_version="0.67.0"
fi

os=`uname | tr '[:upper:]' '[:lower:]'`
if [[ $os == *"mingw64"* ]]; then
	os=win64
elif [[ $os == *"mingw"* ]]; then
	os=win
elif [[ $os == "darwin" ]]; then
	os=osx
fi

if [ `getconf LONG_BIT` -eq 32 ]; then
    arch="ia32"
else
    arch="x64"
fi

# for rpi
if [ `uname -m` == "armv7l" ]; then
    arch="arm"
fi

# for pinebook, probably also rpi 4
if [ `uname -m` == "aarch64" ]; then
    arch="arm64"
fi

# MSYS: Pick the right architecture depending on whether we're
# running in the 32 or 64 bit version of the MSYS shell.
if [[ $os == "win" ]]; then
    arch="ia32"
elif [[ $os == "win64" ]]; then
    arch="x64"
fi
if [[ $os == "win" || $os == "win64" || $os == "osx" ]]; then
    ext="zip"
else
    ext="tar.gz"
fi

if [[ $os == "win" || $os == "win64" ]]; then
    targetdir="$PROGRAMFILES/Purr Data/bin"
    nwjsconfig="$LOCALAPPDATA/purr-data"
elif [[ $os == "osx" ]]; then
    targetdir="/Applications/Purr-Data.app"
    nwjsconfig="$HOME/Library/Application Support/purr-data"
else
    targetdir="/opt/purr-data/lib/pd-l2ork/bin"
    nwjsconfig="$HOME/.config/purr-data"
fi

if [ -n "$nwjs_version" ]; then

nwjs="nwjs-sdk"
if [[ $os == "win64" ]]; then
    nwjs_dirname=${nwjs}-v${nwjs_version}-win-${arch}
else
    nwjs_dirname=${nwjs}-v${nwjs_version}-${os}-${arch}
fi

nwjs_filename=${nwjs_dirname}.${ext}

if [ ! -f "$nwjs_filename" ]; then
    nwjs_url=https://dl.nwjs.io/v${nwjs_version}/$nwjs_filename
    echo "Fetching the nwjs binary from"
    echo "$nwjs_url"

    if ! wget "$nwjs_url"; then
	echo 1>&2 "Download failed. Exiting."
	exit 1
    fi
else
    echo "$nwjs_filename already exists. Skipping download."
fi

if [[ $os == "win" || $os == "win64" || $os == "osx" ]]; then
    unzip $nwjs_filename
else
    tar -xf $nwjs_filename
fi
rm -rf nw
mv $nwjs_dirname nw

# fix permissions
if [[ $os == "win" || $os == "win64" ]]; then
    echo Skipping permissions check.
elif [[ $os == "osx" ]]; then
    chmod -R a+r nw
    find nw -type f -perm +111 -exec chmod a+x {} +
    find nw -type d -exec chmod a+x {} +
else
    chmod -R a+r nw
    find nw -executable -not -type d -exec chmod a+x {} +
    find nw -type d -exec chmod a+x {} +
fi

# Get rid of the previous nw.js config data which may cause issues when
# downgrading the nw.js version.
# (cf. https://agraef.github.io/purr-data-intro/Purr-Data-Intro.html#where-are-my-configuration-files).
echo "Removing previous $nwjsconfig. It will be re-created."
rm -rf "$nwjsconfig"

echo "Replacing nw.js in $targetdir now."
echo "You may be prompted to enter your administrator password."
echo "NOTE: Pressing Ctr+C leaves the nw folder in the current directory."
echo "NOTE: You can then install it manually or remove it."

if [[ $os == "win" || $os == "win64" ]]; then
    echo "NOTE: This requires gsudo (https://gerardog.github.io/gsudo/)."
    echo "NOTE: Otherwise the automatic installation will fail."
    echo "Continue? (Press Return to continue, Ctrl+C to abort.)"
    read
    /c/Program\ Files/gsudo/Current/gsudo.exe "/bin/bash" -c "rm -rf /c/Program\\ Files/Purr\\ Data/bin/nw && mv nw /c/Program\\ Files/Purr\\ Data/bin"
elif [[ $os == "osx" ]]; then
    echo "Continue? (Press Return to continue, Ctrl+C to abort.)"
    read
    # This is complicated. On Mac, the zip contains an entire application
    # bundle, and we must be careful to only replace the Chromium-specific
    # parts, while keeping the purr-data-specific parts of the bundle intact.
    # Replace the nwjs executable:
    sudo mv nw/nwjs.app/Contents/MacOS/nwjs "$targetdir/Contents/MacOS"
    # Replace the framework:
    sudo rm -rf "$targetdir/Contents/Frameworks"
    sudo mv nw/nwjs.app/Contents/Frameworks "$targetdir/Contents"
    # Replace the Chromium translations (*.lproj dirs):
    sudo rm -rf "$targetdir"/Contents/Resources/*.lproj
    # NOTE: The darwin_app/Makefile makes an attempt to edit these (basically,
    # changing the app name from nwjs to Purr Data), but this doesn't seem to
    # do much, at least not with reasonably recent versions. So we just move
    # these over unchanged.
    sudo mv nw/nwjs.app/Contents/Resources/*.lproj "$targetdir/Contents/Resources"
    # Get rid of the rest (there's various other stuff in there that's not
    # part of the bundle, which we don't need):
    rm -rf nw
else
    echo "Continue? (Press Return to continue, Ctrl+C to abort.)"
    read
    sudo rm -rf "$targetdir/nw"
    sudo chown -R root:root nw
    sudo mv nw "$targetdir"
fi

fi #  -n "$nwjs_version"

# edit the package.json file

if [[ $os == "osx" ]]; then
    targetdir="/Applications/Purr-Data.app/Contents/Resources/app.nw"
    # Mac sed has its peculiarities...
    sed="sed -i ''"
else
    # GNU sed
    sed="sed -i"
fi

pkgfile="$targetdir/package.json"

if [ -z "$nwopt" ]; then
    echo "Done."
    exit 0
elif [ ! -f "$pkgfile" ]; then
    echo 1>&2 "-$nwopt specified, but $pkgfile could not be found."
    exit 1
fi

if ! cp "$pkgfile" .; then
    echo 1>&2 "Couldn't create package.json. Exiting."
    exit 1
fi

case "$nwopt" in
    nw1) echo "Turning NW2 mode OFF (https://nwjs.io/blog/nw2-mode/).";
	 echo "You may be prompted to enter your administrator password.";
	 $sed 's/"--proxy-server=/"--disable-features=nw2 --proxy-server=/' package.json;;
    nw2) echo "Turning NW2 mode ON (https://nwjs.io/blog/nw2-mode/).";
	 echo "You may be prompted to enter your administrator password.";
	 echo "WARNING: This may SLOW DOWN the application's user interface.";
	 $sed 's/"--disable-features=nw2 /"/' package.json;;
esac

if [[ $os == "win" || $os == "win64" ]]; then
    echo "NOTE: This requires gsudo (https://gerardog.github.io/gsudo/)."
    echo "NOTE: Otherwise the automatic installation will fail."
    echo "Continue? (Press Return to continue, Ctrl+C to abort.)"
    read
    /c/Program\ Files/gsudo/Current/gsudo.exe "/bin/bash" -c "rm -f /c/Program\\ Files/Purr\\ Data/bin/package.json && mv package.json /c/Program\\ Files/Purr\\ Data/bin"
else
    echo "Continue? (Press Return to continue, Ctrl+C to abort.)"
    read
    sudo rm -f "$pkgfile"
    sudo mv package.json "$targetdir"
fi

echo "Done."
