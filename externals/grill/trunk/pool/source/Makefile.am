#
# automake template
#

# shared library to be built
lib_LTLIBRARIES = @PACKAGE_NAME@.la

@PACKAGE_NAME@_la_SOURCES = pool.h main.cpp pool.cpp data.cpp

@PACKAGE_NAME@_la_CXXFLAGS = @EXT_CFLAGS@ $(patsubst %,-I%,@INCLUDEDIRS@) 

# -module allows to avoid lib prefix for libraries
# -shrext sets shared library extension as given by @EXTENSION@
# -avoid-version avoids the creation of symlinks to versions of a shared library
@PACKAGE_NAME@_la_LDFLAGS = @EXT_LDFLAGS@ -no-undefined -module -shrext @EXTENSION@ -avoid-version $(patsubst %,-L%,@LIBDIRS@) $(patsubst %,-l%,@libs@) $(patsubst %,-framework %,@FRAMEWORKS@) 
#@PACKAGE_NAME@_la_LIBADD = @libs@ 
