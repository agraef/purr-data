flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

----------------------------------------------------------------------------

At the moment this is more like a sketchboard, but i'll promise to bring some
order into it some fine day in the not too distant future.

----------------------------------------------------------------------------

Build scripts
=============

Depending on platform the build process is run with
build.sh
or
build.bat


Arguments: PLATFORM SYSTEM COMPILER {TARGET} {definitions}

PLATFORM: win / lnx / mac
SYSTEM:   pd / max
COMPILER: msvc / gcc / mingw / cygwin / bcc / icc

TARGET: (default is all)
	all, build - build package in default style
	install - install package
	clean - clean build products

	config - test if configuration needs refreshing

or
	build-MODE-TYPE
	install-MODE-TYPE
	clean-MODE-TYPE

with
	MODE: default, all, release, debug, profile
	TYPE: default, all, single, multi, shared


Additional definitions can be passed to the make program
like
	"PKGINFO=info.txt"   (defines new filename for package information)
or
	"PKGINFO="           (package information will be skipped - only for config target)

For more macro names, see below


For each of the supported combinations of PLATFORM, SYSTEM and COMPILER 
a MAKE program has been chosen, normally the one that comes with the compiler.

For gcc it is GNU make (gnumake)
For msvc it is Microsoft make (nmake)
For bcc it is Borland make (bmake)



Package info (package.txt)
==========================

Package information contains vital information for the build process.
Obligatory are only:

NAME: resulting filename of the build product
SRCS: list of source files


Normally also used are:

HDRS: used header files, which SRCS files are dependent upon
SRCDIR: source folder (relative to project folder), default is .


Other settings:

PRECOMPILE: prefix header file (in SRCDIR) for all source files, 
	will be precompiled if supported by the compiler

BUILDCLASS: can currently be flext or ext, default is ext.
	flext will build the flext system
	ext will build a flext-based external

BUILDMODE: release or debug, default is release
	if release, optimization flags will be used
	if debug, debug information will be generated
	if profile, profiling information will be generated (with debug info and optimization)

BUILDTYPE: single, multi or shared, default is single
	if single, it will be linked against the single-threaded static flext library
	if multi, it will be linked against the multi-threaded static flext library
	if shared, it will be linked against the shared flext library

BUILDDIR: relative folder with additional build settings



Additional build settings (BUILDDIR)
====================================

If BUILDDIR is defined, all PLATFORM-SYSTEM-COMPILER combinations to support
must are mirrored by the respective .def and .inc files in the BUILDDIR.


config-PLATFORM.def files (e.g. config-lnx.def) :

	These files can contain additional macro definitions, that are private
	to the project.
	The definitions should be strictly in the form SETTING=value, without any
	make-specific macros etc.
	The .def files work as templates that get copied to a user-editable 
	config.txt file when the build process is first started.


MAKE-PLATFORM-COMPILER.inc files 
(e.g. gnumake-lnx-gcc.inc or nmake-win-msvc.inc):

	These files (which are no considered to be edited by the user) can contain 
	specific modifications to compiler flags, include file paths etc.

	For gnumake this would e.g. be
	INCPATH += -I/usr/local/include/python2.3

	for nmake or bmake e.g.
	INCPATH = $(INCPATH) -I"c:\program files\Python2.3\include"


Structure of build system
=========================

The build system has several levels of information, which are evaluated in the
following order (see also buildsys/MAKE-sub.mak)

Project level:
- PKGINFO file (e.g. package.txt)
- USRCONFIG file (e.g. config.txt)
- USRMAKE file (e.g. build/gnumake-lnx-gcc.inc)

General definitions (in buildsys):

- MAKE.inc (e.g. buildsys/gnumake.inc)
	contains evaluation of flext library name, build directory etc.
- MAKE-BUILDCLASS.inc (e.g. buildsys/gnumake-ext.inc)
	contains some more flag settings

Real-time-system-dependent definitions (in buildsys/PLATFORM/SYSTEM):

- MAKE-COMPILER.inc (e.g. buildsys/lnx/pd/gnumake-gcc.inc)
	contains general real-time-system dependent info (e.g. paths, FLEXT_SYS setting)
- MAKE-COMPILER-BUILDCLASS.inc (e.g. buildsys/lnx/pd/gnumake-gcc-ext.inc)
	contains specific real-time-system dependent info (e.g. extension of binary)

Platform-dependent definitions (in buildsys/PLATFORM):

- MAKE-COMPILER.inc (e.g. buildsys/lnx/gnumake-gcc.inc)
	contains general platform-specific flags
- MAKE-COMPILER-BUILDCLASS.inc (e.g. buildsys/lnx/gnumake-gcc-ext.inc)
	contains the actual make targets (_build_,_clean_ and _install_)


Macro names
===============

PKGINFO - filename for package information (must reside in project folder)

UFLAGS - user defined compiler flags
OFLAGS - user defined optimization flags (not used in debug builds)

CFLAGS - compiler flags
LDFLAGS - linker flags

INCPATH - include file path (must come with e.g. -I )
LIBPATH - library path (must come with e.g. -L )

LIBS - libraries to link in (must come with e.g. -l )
