mkdir debug-nmake-x86_64
cd    debug-nmake-x86_64
cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=Debug -D PKG_OS_ARCH=x86_64 -D CMAKE_INSTALL_PREFIX=../runtime ../..
cd ..

mkdir release-nmake-x86_64
cd    release-nmake-x86_64
cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=Release -D PKG_OS_ARCH=x86_64 -D CMAKE_INSTALL_PREFIX=../runtime ../..
cd ..
