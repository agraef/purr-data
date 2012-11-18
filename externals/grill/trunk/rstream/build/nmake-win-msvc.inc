INCPATH=/I$(VORBISPATH)\include /I$(OGGPATH)\include /I$(PTHREADPATH)\include
LIBPATH=/LIBPATH:$(PTHREADPATH)\lib
LIBS=ws2_32.lib pthreadVC2.lib

!ifdef VC98

!ifdef DYNAMIC
LIBPATH=$(LIBPATH) /LIBPATH:$(VORBISPATH)\win32\vorbis_dynamic_release /LIBPATH:$(VORBISPATH)\win32\vorbisfile_dynamic_release /LIBPATH:$(OGGPATH)\win32\dynamic_release
LIBS=$(LIBS) vorbis.lib vorbisfile.lib ogg.lib 
!else
LIBPATH=$(LIBPATH) /LIBPATH:$(VORBISPATH)\win32\vorbis_static_release /LIBPATH:$(VORBISPATH)\win32\vorbisfile_static_release /LIBPATH:$(OGGPATH)\win32\static_release
LIBS=$(LIBS) vorbis_static.lib vorbisfile_static.lib ogg_static.lib pthreadVC2.lib
!endif

!else

LIBPATH=$(LIBPATH) /LIBPATH:$(VORBISPATH)\win32\VS2003\libvorbis\Release /LIBPATH:$(VORBISPATH)\win32\VS2003\libvorbisfile\Release /LIBPATH:$(OGGPATH)\win32\VS2003\libogg\Release
LIBS=$(LIBS) libvorbis.lib libvorbisfile.lib libogg.lib 

!endif
