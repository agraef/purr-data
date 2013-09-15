@echo off
REM ==========================================================================
REM  Run cmake with a suitable (i.e. installed) generator!
REM ==========================================================================
REM The following generators are available on this platform:
REM   Borland Makefiles               = Generates Borland makefiles.
REM   MSYS Makefiles                  = Generates MSYS makefiles.
REM   MinGW Makefiles                 = Generates a make file for use with mingw32-make.
REM   NMake Makefiles                 = Generates NMake makefiles.
REM   Unix Makefiles                  = Generates standard UNIX makefiles.
REM   Visual Studio 6                 = Generates Visual Studio 6 project files.
REM   Visual Studio 7                 = Generates Visual Studio .NET 2002 project files.
REM   Visual Studio 7 .NET 2003       = Generates Visual Studio .NET 2003 project files.
REM   Visual Studio 8 2005            = Generates Visual Studio .NET 2005 project files.
REM   Visual Studio 8 2005 Win64      = Generates Visual Studio .NET 2005 Win64 project files.
REM   Visual Studio 9 2008            = Generates Visual Studio 9 2008 project files.
REM   Visual Studio 9 2008 Win64      = Generates Visual Studio 9 2008 Win64 project files.
REM   Watcom WMake                    = Generates Watcom WMake makefiles.
REM   CodeBlocks - MinGW Makefiles    = Generates CodeBlocks project files.
REM   CodeBlocks - Unix Makefiles     = Generates CodeBlocks project files.
REM   Eclipse CDT4 - MinGW Makefiles  = Generates Eclipse CDT 4.0 project files.
REM   Eclipse CDT4 - NMake Makefiles  = Generates Eclipse CDT 4.0 project files.
REM   Eclipse CDT4 - Unix Makefiles   = Generates Eclipse CDT 4.0 project files.

mkdir debug-nmake-win32
cd    debug-nmake-win32
cmake -G "NMake Makefiles" -D CMAKE_BUILD_TYPE=Debug -D CMAKE_INSTALL_PREFIX=../runtime ..\..
cd ..

mkdir release-nmake-win32
cd    release-nmake-win32
cmake -G "NMake Makefiles" -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=../runtime ..\..
cd ..
