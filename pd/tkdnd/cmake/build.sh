mkdir debug-nmake-x86_32
cd    debug-nmake-x86_32
cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=Debug -D PKG_OS_ARCH=x86_32 -D CMAKE_INSTALL_PREFIX=../runtime ../..
cd ..

mkdir release-nmake-x86_32
cd    release-nmake-x86_32
cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=Release -D PKG_OS_ARCH=x86_32 -D CMAKE_INSTALL_PREFIX=../runtime ../..
cd ..
