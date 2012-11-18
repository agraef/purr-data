@echo off

rem
rem flext - C++ layer for Max/MSP and pd (pure data) externals
rem
rem Copyright (c) 2001-2005 Thomas Grill (gr@grrrr.org)
rem For information on usage and redistribution, and for a DISCLAIMER OF ALL
rem WARRANTIES, see the file, "license.txt," in this distribution.  
rem
rem more information on http://grrrr.org/ext
rem ------------------------------------------------------------------------
rem
rem To build flext or flext-based externals simply run this script.
rem Running it without arguments will print some help to the console.
rem
rem ------------------------------------------------------------------------

echo ------------------------------------------------

set flext=%~dp0

rem Arguments:
rem %1 - system (pd/max)
rem %2 - compiler (msvc/gcc/mingw/cygwin/bcc/icc)
rem %3 - target (build/clean/install)

set platform=win
set rtsys=%1
set compiler=%2
set target=%3

rem --- The subbatch knowns which make utility to use ---
set subbatch=%flext%\buildsys\build-%compiler%.bat

if "%platform%"=="" goto syntax
if "%rtsys%"=="" goto syntax
if "%compiler%"=="" goto syntax

if not exist "%subbatch%" goto syntax

call "%subbatch%" %platform% %rtsys% %target% %4 %5 %6 %7 %8 %9

goto end

rem -----------------------------------------
:syntax

echo .
echo SYNTAX: build [system] [compiler] {target}
echo system   ... pd / max
echo compiler ... msvc / gcc / mingw / cygwin / bcc / icc
echo target   ... all (default) / clean / install
echo .
echo Please make sure that your make program and compiler can be accessed with the
echo system path and that all relevant environment variables are properly set.
echo .
echo For further information read flext/build.txt
echo .

:end
