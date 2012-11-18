#!/bin/sh

# this script is no longer maintained, instead use the 'sources tree and this script:
# https://pure-data.svn.sourceforge.net/svnroot/pure-data/sources/build-libs-on-mingw.sh

# This script builds everything needed to build Pd-extended on MinGW.  You
# need to download all of the source files listed on
# http://puredata.org/docs/developer/win first, put them all into one
# directory, then run this script in that directory.  It should build and
# install everything.  Make sure you have this line in your
# c:\msys\1.0\etc\fstab:

# c:\MinGW  /usr/local

# This ensures that everything will be installed in the right
# place. <hans@eds.org>

# pthreads
testfile=/usr/local/bin/pthreadGC2.dll
if [ -e "$testfile" ]; then 
	 echo "$testfile exists, skipping..."
else
	 echo "Building everything for $testfile"
	 cd pthreads.2
	 make clean GC-inlined
	 cp libpthreadGC2.a /usr/local/lib
	 cp pthreadGC2.dll /usr/local/bin
	 cp pthread.h sched.h semaphore.h /usr/local/include/
	 cd ..
fi

# Tcl
testfile=/usr/local/bin/tcl84.dll
if [ -e "$testfile" ]; then 
	 echo "$testfile exists, skipping..."
else
	 echo "Building everything for $testfile"
	 tar xzf tcl8.4.*-src.tar.gz
	 cd tcl8.4.*/win
	 ./configure && make CYGPATH=echo && make install
	 cd ../..
fi

# Tk
testfile=/usr/local/bin/tk84.dll
if [ -e "$testfile" ]; then 
	 echo "$testfile exists, skipping..."
else
	 echo "Building everything for $testfile"
	 tar xzf tk8.4.*-src.tar.gz
	 cd tk8.4.*/win
	 ./configure && make CYGPATH=echo && make install
	 cd ../..
fi

# ogg
testfile=/usr/local/lib/libogg.a
if [ -e "$testfile" ]; then 
	 echo "$testfile exists, skipping..."
else
	 echo "Building everything for $testfile"
	 tar xzf libogg-1.1.*.tar.gz
	 cd libogg-1.1.*
	 ./configure && make && make install
	 cd ..
fi


# GNU regex
testfile=/usr/local/lib/libregex.a
if [ -e "$testfile" ]; then 
	 echo "$testfile exists, skipping..."
else
	 echo "Building everything for $testfile"
	 tar xzf regex-0.12.tar.gz
	 cd regex-0.12
	 ./configure && make
	 ar ru libregex.a regex.o
	 cp libregex.a /usr/local/lib
	 cp regex.h /usr/local/include
	 cd ..
fi


# vorbis
testfile=/usr/local/lib/libvorbisfile.a
if [ -e "$testfile" ]; then 
	 echo "$testfile exists, skipping..."
else
	 echo "Building everything for $testfile"
	 tar xzf libvorbis-1.1.*.tar.gz
	 cd libvorbis-1.1.*
	 ./configure && make
	 cd lib
	 /bin/sh ../libtool --tag=CC --mode=link gcc  -O20 -D__NO_MATH_INLINES \
		  -fsigned-char  -DUSE_MEMORY_H   -o libvorbisfile.la -rpath \
		  /usr/local/lib -no-undefined -version-info 4:0:1 vorbisfile.lo \
		  libvorbis.la /usr/local/lib/libogg.la 
	 cd ..
	 make && make install
	 cd ..
fi

# LAME
testfile=/usr/local/bin/libmp3lame-0.dll
if [ -e "$testfile" ]; then 
	 echo "$testfile exists, skipping..."
else
	 echo "Building everything for $testfile"
	 tar xzf lame-398-2.tar.gz
	 cd lame-398-2
	 ./configure --disable-frontend \
		 && make \
		 && make install
	 cd ..
fi

# speex
testfile=/usr/local/lib/libspeex.a
if [ -e "$testfile" ]; then 
	 echo "$testfile exists, skipping..."
else
	 echo "Building everything for $testfile"
	 tar xzf speex-*.tar.gz
	 cd speex-*
	 ./configure --enable-sse && make && make install
	 cd ..
fi

# FLAC
testfile=/usr/local/lib/libFLAC.a
if [ -e "$testfile" ]; then 
	 echo "$testfile exists, skipping..."
else
	 echo "Building everything for $testfile"
	 tar xzf flac-1.1.*.tar.gz && cd flac-1.1.*
	 ./configure && make && make install || echo -e "\n\n$testfile failed!!\n\n"
# the compilation bombs, but builds most of what we need, so install anyway
	 make -k install
	 cd ..
fi

# libsndfile
# the FLAC build bombs, so disable FLAC support in libsndfile
testfile=/usr/local/lib/libsndfile.a
if [ -e "$testfile" ]; then 
	 echo "$testfile exists, skipping..."
else
	 echo "Building everything for $testfile"
	 tar xzf libsndfile-1.0.19.tar.gz
	 cd libsndfile-1.0.19
	 ./configure --disable-alsa --enable-sqlite 
		 && make && make check && make install
	 cd ..
fi

# fftw3
testfile=/usr/local/lib/libfftw3.a
if [ -e "$testfile" ]; then 
	 echo "$testfile exists, skipping..."
else
	 echo "Building everything for $testfile"
	 tar xzf fftw-3.1.*.tar.gz
	 cd fftw-3.1.*
	 ./configure --with-our-malloc16 --with-windows-f77-mangling --enable-shared --disable-static --enable-threads --with-combined-threads --enable-portable-binary --enable-float --enable-sse && \
	     make && make install
	 cd ..
fi

