pdoctave:

pd-octave shall be an interface to octave, mainly in order to access octaves command-line functionality and enormous function space.

This idea was started approximately in February 2005. 

I want to appologize for the limited flexibility of the code at the moment. 
That's why the use of this external is only recommended to developers.

Following packages have to be installed:
puredata (+sources)
octave
octave-headers

Nice to have:
bbogart's "entry" pd-external

INSTALLING:
please edit the linker parameters in "Makefile" to match your pd-sources paths
type "make"

ADD THE FOLLOWING LINE TO YOUR ~/.octaverc (Pathdefinition) and modify the path to your personal pdoctave path (the second slash is mandatory)
'  LOADPATH = ":/home/franz/pdoctave//"  '

TESTING:
"pd -lib pdoctave pdoctave-help.pd"

This code collection consists of four pd-objects:

1. pdoctave: an object that calls octave and holds a pipe to its std in. this pipe can be acessed through the other pd-objects.

2. pd-command: sending commands (strings) to the octave command line
3. pd-send:    creating variables on the octave workspace
4. pd-get:     calling back variables from the octave workspace

In order to communicate with octave, there are two .cc files that are compiled in octave external language (mkoctfile)
5. read_shared_mem.cc
6. write_shared_mem.cc

The variables use a common header format with access functions and definitions in
7. pdoctave-dataframe.c/h
8. pdoctave-datatype.h


Franz Zotter 22.Feb 2006 
(Feb 2005)

