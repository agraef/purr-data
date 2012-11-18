#--------------------------------------------------------------

current: gripd_linux
clean: clean_linux

#----------- Options (all OS) --------------------------------

MIDI = TRUE
#MIDI = FALSE
JOYSTICK = TRUE
#JOYSTICK = FALSE

#----------- Settings (Linux) --------------------------------

MIDIOSSUBTYPE = ALSA
#MIDIOSSUBTYPE = OSS
PDINCLUDE =  -I../../src -I../../../pd/src
LINUXPYTHONINCLUDE = -I/usr/include/python2.5
SYSTEMLIBDIR = /usr/lib

#----------- Settings (Windows) -------------------------------

VC = "C:\Program Files\Microsoft Visual Studio\VC98"
PYTHONHOME = C:\python22
PYTHONVER = 22
PDNTSRCDIR = C:\pd-37\src
PDNTLDIR = C:\pd-37\bin
  
#----------- Linux --------------------------------------------

gripd_linux: 
	make -f Makefile.Linux all
clean_linux: 
	make -f Makefile.Linux linux_clean

#----------- Win32---------------------------------------------

gripd_nt: 
	nmake -f Makefile.NT all
clean_nt: 
	nmake -f Makefile.NT nt_clean



