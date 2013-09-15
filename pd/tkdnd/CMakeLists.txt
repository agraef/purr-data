CMAKE_MINIMUM_REQUIRED ( VERSION 2.6 )

## Ensure that the variable PKG_OS_ARCH exists! If undefined, define it with
## the value "x86_32"
IF ( NOT PKG_OS_ARCH )
  SET ( PKG_OS_ARCH x86_32 )
ENDIF ( NOT PKG_OS_ARCH )

## ===========================================================================
##  No need to modify below this point!
## ===========================================================================

SET ( NO_MSVCRT ON )

## ===========================================================================
##  Project Information
## ===========================================================================
PROJECT ( tkdnd )
## Package version information:
SET ( PKG_NAME          ${PROJECT_NAME} )
SET ( PKG_MAJOR_VERSION 2 )
SET ( PKG_MINOR_VERSION 6 )
SET ( PKG_BUILD_VERSION 0 )
## Author:
SET ( PKG_VENDOR        "Georgios Petasis" )

## ===========================================================================
##  Project sources
## ===========================================================================
IF ( WIN32 )
  ## True under windows (32bit, 64bit, Cygwin)...
  INCLUDE_DIRECTORIES ( win )
  SET ( CMAKE_SHARED_LIBRARY_PREFIX lib )
  SET ( PKG_SOURCES win/TkDND_OleDND.cpp )

  ADD_DEFINITIONS     ( -DUNICODE )
  ADD_DEFINITIONS     ( -D_UNICODE )
  # ADD_DEFINITIONS     ( -D_MBCS )

ELSE  ( WIN32 )
  ## Unix and OS X...
  IF ( APPLE )
    INCLUDE_DIRECTORIES ( macosx )
    ADD_DEFINITIONS     ( -DMAC_TK_COCOA )
    ADD_DEFINITIONS     ( -std=gnu99 )
    ADD_DEFINITIONS     ( -x objective-c )
    ADD_DEFINITIONS     ( -fobjc-gc )
    LINK_LIBRARIES      ( -framework Cocoa -framework Carbon )
    SET ( PKG_SOURCES unix/macosx/macdnd.m )
  ELSE ( APPLE )
    INCLUDE_DIRECTORIES ( unix )
    SET ( PKG_SOURCES unix/TkDND_XDND.c unix/tkUnixSelect.c unix/Cursors.c )
    MESSAGE ( STATUS "Searching for X11..." )
    FIND_PACKAGE ( X11 REQUIRED )
    IF ( X11_FOUND )
      INCLUDE_DIRECTORIES ( ${X11_INCLUDE_DIR} )
      LINK_LIBRARIES      ( ${X11_LIBRARIES} )
      MESSAGE ( STATUS "  X11_INCLUDE_DIR:         " ${X11_INCLUDE_DIR} )
      MESSAGE ( STATUS "  X11_LIBRARIES:           " ${X11_LIBRARIES} )
    ENDIF ( X11_FOUND )
  ENDIF ( APPLE )
ENDIF ( WIN32 )

##
## If we are building with MINGW, link statically to its libs...
##
IF ( MINGW )
  INCLUDE ( CheckCCompilerFlag )
  CHECK_C_COMPILER_FLAG ( -static-libgcc    FLAG_static_libgcc    )
  CHECK_C_COMPILER_FLAG ( -static-libstdc++ FLAG_static_libstdcpp )
  IF ( FLAG_static_libgcc )
    MESSAGE ( STATUS " -> MINGW detected: adding -static-libgcc!" )
    SET ( CMAKE_SHARED_LINKER_FLAGS
       "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc" )
  ENDIF ( FLAG_static_libgcc )
  IF ( FLAG_static_libstdcpp )
    MESSAGE ( STATUS " -> MINGW detected: adding -static-libstdc++!" )
    SET ( CMAKE_SHARED_LINKER_FLAGS
       "${CMAKE_SHARED_LINKER_FLAGS} -static-libstdc++" )
  ENDIF (FLAG_static_libstdcpp )
ENDIF ( MINGW )

## Arange project version information...
SET ( PKG_VERSION
     "${PKG_MAJOR_VERSION}.${PKG_MINOR_VERSION}.${PKG_BUILD_VERSION}" )
SET ( PKG_VERSION
     "${PKG_MAJOR_VERSION}.${PKG_MINOR_VERSION}" )
SET ( PKG_NAME_VERSION  ${PKG_NAME}-${PKG_VERSION} )
SET ( PKG_HOME_DIR      ${PROJECT_SOURCE_DIR} )

## Greet the user...
MESSAGE ( STATUS "===========================================================" )
MESSAGE ( STATUS " Welcome to the ${PKG_NAME} ${PKG_VERSION} build system!" )
MESSAGE ( STATUS "  * Selected generator:  ${CMAKE_GENERATOR}" )
MESSAGE ( STATUS "  * Operating System ID: "
        ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_VERSION}-${CMAKE_SYSTEM_PROCESSOR} )
MESSAGE ( STATUS "  * Installation Directory: " ${CMAKE_INSTALL_PREFIX} )
MESSAGE ( STATUS "===========================================================" )

## Allow additional cmake modules in the modules directory...
SET ( CMAKE_MODULE_PATH cmake/modules )

## ===========================================================================
##  Utilities
## ===========================================================================
SET ( RM           ${CMAKE_COMMAND} -E remove -f )
SET ( RM_FLAGS     )
SET ( RMDIR        ${CMAKE_COMMAND} -E remove_directory )
SET ( RMDIR_FLAGS  )
SET ( MKDIR        ${CMAKE_COMMAND} -E make_directory )
SET ( CP           ${CMAKE_COMMAND} -E copy )

## ===========================================================================
##  Locate Tcl/Tk
## ===========================================================================
MESSAGE ( STATUS "Searching for Tcl/Tk..." )
FIND_PACKAGE ( TCL REQUIRED )
FIND_PACKAGE ( TclStub REQUIRED )

## Tcl/Tk info (useful for debug purposes)...
# MESSAGE ( STATUS "  TCL_TCLSH:               " ${TCL_TCLSH} )
  MESSAGE ( STATUS "  TCL_INCLUDE_PATH:        " ${TCL_INCLUDE_PATH} )
  MESSAGE ( STATUS "  TCL_STUB_LIBRARY:        " ${TCL_STUB_LIBRARY} )
# MESSAGE ( STATUS "  TCL_LIBRARY:             " ${TCL_LIBRARY} )
# MESSAGE ( STATUS "  TK_WISH:                 " ${TK_WISH} )
  MESSAGE ( STATUS "  TK_INCLUDE_PATH:         " ${TK_INCLUDE_PATH} )
  MESSAGE ( STATUS "  TK_STUB_LIBRARY:         " ${TK_STUB_LIBRARY} )
# MESSAGE ( STATUS "  TK_LIBRARY:              " ${TK_LIBRARY} )

## Enable Tcl/Tk stubs globally...
ADD_DEFINITIONS     ( -DUSE_TCL_STUBS )
ADD_DEFINITIONS     ( -DUSE_TK_STUBS )
ADD_DEFINITIONS     ( -DTCL_THREADS )
INCLUDE_DIRECTORIES ( ${TCL_INCLUDE_PATH} )
INCLUDE_DIRECTORIES ( ${TK_INCLUDE_PATH} )
LINK_LIBRARIES      ( ${TCL_STUB_LIBRARY} )
LINK_LIBRARIES      ( ${TK_STUB_LIBRARY} )

IF ( WIN32 AND NO_MSVCRT )
  STRING ( REPLACE /MD  /MT  CMAKE_C_FLAGS_RELEASE   ${CMAKE_C_FLAGS_RELEASE}  )
  STRING ( REPLACE /MDd /MTd CMAKE_C_FLAGS_DEBUG     ${CMAKE_C_FLAGS_DEBUG}    )
  STRING ( REPLACE /MD  /MT  CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
  STRING ( REPLACE /MDd /MTd CMAKE_CXX_FLAGS_DEBUG   ${CMAKE_CXX_FLAGS_DEBUG}  )
  SET (    CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} /nodefaultlib:MSVCRT.LIB" )
  SET (    CMAKE_EXE_LINKER_FLAGS_RELEASE
        "${CMAKE_EXE_LINKER_FLAGS_RELEASE} LIBCMT.LIB" )
  SET (    CMAKE_EXE_LINKER_FLAGS_DEBUG
        "${CMAKE_EXE_LINKER_FLAGS_DEBUG} LIBCMTD.LIB" )
  SET (    CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} /nodefaultlib:MSVCRT.LIB" )
  SET (    CMAKE_SHARED_LINKER_FLAGS_RELEASE
        "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} LIBCMT.LIB" )
  SET (    CMAKE_SHARED_LINKER_FLAGS_DEBUG
        "${CMAKE_SHARED_LINKER_FLAGS_DEBUG}
         /nodefaultlib:LIBCMT.LIB LIBCMTD.LIB" )
  ADD_DEFINITIONS ( -D_STATIC_CPPLIB )
  ADD_DEFINITIONS ( -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1 )
  ADD_DEFINITIONS ( -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT=1 )
  ADD_DEFINITIONS ( -D_CRT_SECURE_NO_WARNINGS )
ENDIF ( WIN32 AND NO_MSVCRT )

## Package global definitions...
ADD_DEFINITIONS   ( -DPACKAGE_NAME="${PKG_NAME}" )
ADD_DEFINITIONS   ( -DPACKAGE_VERSION="${PKG_VERSION}" )
ADD_DEFINITIONS   ( -DPACKAGE_NAME_VERSION="${PKG_NAME_VERSION}" )
ADD_DEFINITIONS   ( -DTKDND_PACKAGE="${PKG_NAME}" )
ADD_DEFINITIONS   ( -DTKDND_VERSION="${PKG_VERSION}" )

## Package debug definitions...
# ADD_DEFINITIONS   ( -DDEBUG_CLIENTMESSAGE_HANDLER )

## ===========================================================================
##  Declare the package shared library target...
## ===========================================================================
SET ( PKG_TARGET_LIB_NAME ${PKG_NAME}${PKG_VERSION} )
MESSAGE ( STATUS "      + Shared Library: ${PKG_NAME}" )
ADD_LIBRARY ( ${PKG_TARGET_LIB_NAME} SHARED ${PKG_SOURCES} )

## ===========================================================================
##  Generate the pkgIndex.tcl file...
## ===========================================================================
FILE ( WRITE library/pkgIndex.tcl 
"package ifneeded ${PKG_NAME} ${PKG_VERSION} \\
  \"source \\{$dir/tkdnd.tcl\\} ; \\
   tkdnd::initialise \\{$dir\\} lib${PKG_TARGET_LIB_NAME}[info sharedlibextension] ${PKG_NAME}\"" )

## ===========================================================================
##  Declare the package install targets...
## ===========================================================================
INSTALL ( TARGETS ${PKG_TARGET_LIB_NAME}
          RUNTIME DESTINATION ${PKG_TARGET_LIB_NAME}
          LIBRARY DESTINATION ${PKG_TARGET_LIB_NAME}
          ARCHIVE DESTINATION ${PKG_TARGET_LIB_NAME} )
INSTALL ( FILES library/pkgIndex.tcl
                library/tkdnd.tcl
                library/tkdnd_compat.tcl
                library/tkdnd_macosx.tcl
                library/tkdnd_unix.tcl
                library/tkdnd_windows.tcl
          DESTINATION ${PKG_TARGET_LIB_NAME} )
