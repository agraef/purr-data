@set build=%~dp0

@rem workaround for MSVC 2005 command prompt, where LIBPATH is predefined
@set LIBPATH=

@echo on
nmake -f "%build%nmake.mak" PLATFORM=%1 RTSYS=%2 COMPILER=msvc BUILDPATH=%build% %3 %4 %5 %6 %7 %8 %9
