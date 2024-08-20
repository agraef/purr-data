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

# NOTE: The script currently only works on Linux and on Windows using the msys
# shell. On Mac, the nw.js package will be downloaded and extracted, but then
# leaves you to do the rest, because I haven't yet figured out what goes where
# exactly on the Mac; it's complicated. But if you're lucky, replacing just
# /Applications/Purr-Data/Contents/MacOS/nwjs with the downloaded binary in
# nw/nwjs.app/Contents/MacOS/nwjs might do the trick.

nwjs_version="0.67.0"
case "$1" in
    [0-9]*) nwjs_version="$1" ;;
    "") ;;
    *) echo "USAGE: $0 [-h] [nwjs-version]" 1>&2
       echo "-h: print this message" 1>&2
       echo "nwjs-version (x.y.z): nw.js version to be used (default: 0.67.0)" 1>&2
       exit 0 ;;
esac

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

if [[ $os == "win" || $os == "win64" ]]; then
    targetdir="$PROGRAMFILES/Purr-Data/bin"
    nwjsconfig="$LOCALAPPDATA/purr-data"
elif [[ $os == "osx" ]]; then
    targetdir="/Applications/Purr-Data.app"
    nwjsconfig="$HOME/Library/Application Support/purr-data"
else
    targetdir="/opt/purr-data/lib/pd-l2ork/bin"
    nwjsconfig="$HOME/.config/purr-data"
fi

# Get rid of the previous nw.js config data which may cause issues when
# downgrading the nw.js version.
# (cf. https://agraef.github.io/purr-data-intro/Purr-Data-Intro.html#where-are-my-configuration-files).
echo "Removing previous $nwjsconfig. It will be re-created."
rm -rf "$nwjsconfig"

if [[ $os == "win" || $os == "win64" ]]; then
    echo "Replacing nw.js in $targetdir now."
    echo "You may be prompted to enter your administrator password."
    echo "NOTE: Pressing Ctr+C leaves the nw folder in the current directory."
    echo "NOTE: You can then install it manually or remove it."
    echo "NOTE: This requires gsudo (https://gerardog.github.io/gsudo/)."
    echo "NOTE: Otherwise the automatic installation will fail."
    echo "Continue? (Press Return to continue, Ctrl+C to abort.)"
    read
    /c/Program\ Files/gsudo/Current/gsudo.exe "/bin/bash" -c "rm -rf /c/Program\\ Files/Purr\\ Data/bin/nw && mv nw /c/Program\\ Files/Purr\\ Data/bin"
elif [[ $os == "osx" ]]; then
    echo "Replacing nw.js in $targetdir now."
    echo "You may be prompted to enter your sudo password."
    echo "NOTE: Pressing Ctr+C leaves the nw folder in the current directory."
    echo "NOTE: You can then install it manually or remove it."
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
    echo "Replacing nw.js in $targetdir now."
    echo "You may be prompted to enter your sudo password."
    echo "NOTE: Pressing Ctr+C leaves the nw folder in the current directory."
    echo "NOTE: You can then install it manually or remove it."
    echo "Continue? (Press Return to continue, Ctrl+C to abort.)"
    read
    sudo rm -rf "$targetdir/nw"
    sudo chown -R root:root nw
    sudo mv nw "$targetdir"
fi
