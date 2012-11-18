# customize here !
#VC="C:\Programmi\Microsoft Visual Studio .NET\Vc7"
#VC="C:\Programmi\Microsoft Visual Studio .NET\Vc7"
VC="C:\Programmi\Microsoft Platform SDK"
VC="C:\Programmi\Microsoft Visual C++ Toolkit 2003"
#PDPATH="H:\PureData\pd-0.38-3.msw\pd"
#PDPATH="C:\Documents and Settings\Davide\Documenti\personali\pd-0.38-3.msw\pd"
PDPATH="C:\Programmi\pd\pd"

current: pd_nt distclean

pd_nt: chord_melo.dll chords_memory.dll harmonizer.dll voicing_analyzer.dll GArhythm.dll ritmo1.dll rhythms_memory.dll themes_memory.dll

.SUFFIXES: .dll

PDNTCFLAGS = /W3 /WX /O2 /G6 /DNT /DPD /nologo

PDNTINCLUDE = /I. /I$(PDPATH)\tcl\include /I$(PDPATH)\src /I$(VC)\include /Iinclude

PDNTLDIR = $(VC)\Lib
PDNTLIB = $(PDNTLDIR)\libc.lib \
	$(PDNTLDIR)\oldnames.lib \
	$(PDNTLDIR)\kernel32.lib \
	$(PDPATH)\bin\pd.lib 

.c.dll:
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c common.c $*.c
	link /dll /export:$*_setup $*.obj common.obj $(PDNTLIB) 



clean:
	-del link.stamp
	-del *.obj
	-del *.lib
	-del *.exp
	-del *.dll

distclean:
	-del *.obj
	-del *.lib
	-del *.exp
