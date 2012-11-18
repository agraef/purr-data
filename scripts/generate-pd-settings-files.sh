#!/bin/sh

ECHO=/sw/bin/echo
LIBS="libdir Gem cyclone zexy creb cxc iemlib list-abs mapping markex maxlib memento mjlib motex oscx pddp pdogg pixeltango pmpd rradical sigpack smlib toxy unauthorized vbap pan hcs jmmmp ext13 ggee iem_anything flib ekext flatspace pdp pidip"

GNULINUX_FONTPATH="/var/lib/defoma/x-ttcidfont-conf.d/dirs/TrueType"
MACOSX_FONTPATH="/System/Library/Fonts /Library/Fonts ~/Library/Fonts /usr/X11R6/lib/X11/fonts/TTF /System/Library/Frameworks/JavaVM.framework/Versions/1.5.0/Home/lib/fonts"
# these are set as hex(2) since the .reg format doesn't support REG_EXPAND_SZ.
# Inno Setup doesn't convert REG_SZ hex values to REG_EXPAND_SZ, so Pd can't
# read the values then.  Therefore there is a separate set for InnoSetup to
# use to create REG_EXPAND_SZ VALUES
# path1 = %SystemRoot%/Fonts
WINDOWS_INNO_PATH="%SystemRoot%/Fonts"
WINDOWS_REG_PATH="hex(2):25,00,53,00,79,00,73,00,74,00,65,00,6d,00,52,00,6f,00,6f,00,74,00,25,00,2f,00,46,00,6f,00,6e,00,74,00,73,00,00,00"

SCRIPT_DIR=`${ECHO} $0 | sed 's|\(.*\)/.*$|\1|'`
ROOT_DIR=${SCRIPT_DIR}/../packages

GNULINUX_FILE=${ROOT_DIR}/linux_make/default.pdl2ork
MACOSX_FILE=${ROOT_DIR}/darwin_app/org.puredata.pdl2ork.default.plist
WINDOWS_FILE=${ROOT_DIR}/win32_inno/pd-settings.reg
WINDOWS_INNO_FILE=${ROOT_DIR}/win32_inno/pd-inno.iss.in
WINDOWS_INNO_REG_FILE=${ROOT_DIR}/win32_inno/pd-inno.registry.reg

GNULINUX_HEADER='standardpath: 1\nverbose: 0\ndefeatrt: 0\nflags:'


MACOSX_HEADER='<?xml version="1.0" encoding="UTF-8"?>\n<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">\n<plist version="1.0">\n<dict>\n\t<key>defeatrt</key>\n\t<string>0</string>\n\t<key>flags</key>\n\t<string>-helppath ~/Library/Pd -helppath /Library/Pd</string>'
MACOSX_FOOTER='</dict>\n
</plist>\n'


WINDOWS_HEADER='Windows Registry Editor Version 5.00\n\n[HKEY_LOCAL_MACHINE\SOFTWARE\Pd-extended]'


# the file associations should be added here
WINDOWS_INNO_HEADER=''


# GNU/Linux -------------------------------------------------------------------#
print_gnulinux ()
{
	 ${ECHO} "loadlib$1: $2" >> $GNULINUX_FILE
}

print_gnulinux_fontpath ()
{
	 i=0
	 IFS=' '
	 for fontpath in $GNULINUX_FONTPATH; do
		  ((++i)) 
		  ${ECHO} "path${i}: ${fontpath}" >> $GNULINUX_FILE
	 done
	 ${ECHO} "npath: ${i}" >> $GNULINUX_FILE
}

print_gnulinux_nloadlib ()
{
	 ${ECHO} "nloadlib: $1" >> $GNULINUX_FILE
}

# Mac OS X --------------------------------------------------------------------#
print_macosx () 
{
	 ${ECHO} -e "\t<key>loadlib$1</key>" >> $MACOSX_FILE
	 ${ECHO} -e "\t<string>$2</string>" >> $MACOSX_FILE
}

print_macosx_fontpath ()
{
	 i=0
	 IFS=' '
	 for fontpath in $MACOSX_FONTPATH; do
		  ((++i)) 
		  ${ECHO} -e "\t<key>path${i}</key>" >> $MACOSX_FILE
		  ${ECHO} -e "\t<string>${fontpath}</string>" >> $MACOSX_FILE
	 done
	 ${ECHO} -e "\t<key>npath</key>" >> $MACOSX_FILE
	 ${ECHO} -e "\t<string>${i}</string>" >> $MACOSX_FILE
}

print_macosx_nloadlib ()
{
	${ECHO} -e "\t<key>nloadlib</key>" >> $MACOSX_FILE
	${ECHO} -e "\t<string>${1}</string>" >> $MACOSX_FILE
}

# Windows ---------------------------------------------------------------------#
print_windows ()
{
	 ${ECHO} "\"loadlib$1\"=\"$2\"" >> $WINDOWS_FILE
	 ${ECHO} "Root: HKLM; SubKey: SOFTWARE\Pd-extended; ValueType: string; ValueName: loadlib$1; ValueData: $2; Tasks: libs"  >> $WINDOWS_INNO_REG_FILE
}

print_windows_delete ()
{
	${ECHO} "\"${1}${2}\"=-" >> $WINDOWS_FILE
	${ECHO} "Root: HKLM; SubKey: SOFTWARE\Pd-extended; ValueType: none; ValueName: ${1}${2}; Flags: deletevalue; Tasks: libs"  >> $WINDOWS_INNO_REG_FILE
}

print_windows_helppath ()
{
	${ECHO} "\"flags\"=\"\"" >> $WINDOWS_FILE
	${ECHO} "Root: HKLM; SubKey: SOFTWARE\Pd-extended; ValueType: string; ValueName: flags; ValueData: ""; Tasks: libs; Flags: uninsdeletekey" >> $WINDOWS_INNO_REG_FILE
}

print_windows_inno_path ()
{
	j=0
	IFS=';'
	for fontpath in $WINDOWS_INNO_PATH; do
		((++j)) 
		${ECHO} "Root: HKLM; SubKey: SOFTWARE\Pd-extended; ValueType: expandsz; ValueName: path${j}; ValueData: ${fontpath}; Tasks: libs; Flags: uninsdeletekey" >> $WINDOWS_INNO_REG_FILE
	done
	${ECHO} "Root: HKLM; SubKey: SOFTWARE\Pd-extended; ValueType: string; ValueName: npath; ValueData: ${j}; Tasks: libs; Flags: uninsdeletekey" >> $WINDOWS_INNO_REG_FILE
# print lines to delete existing path flags
	${ECHO} "; delete all existing path flags" >> $WINDOWS_FILE
	while [ $j -lt 100 ]; do
		((++j)) 
		${ECHO} "Root: HKLM; SubKey: SOFTWARE\Pd-extended; ValueType: none; ValueName: path${j}; Flags: deletevalue; Tasks: libs"  >> $WINDOWS_INNO_REG_FILE
	done
}

print_windows_reg_path ()
{
	j=0
	IFS=' '
	for fontpath in $WINDOWS_REG_PATH; do
		((++j)) 
		${ECHO} "\"path${j}\"=${fontpath}" >> $WINDOWS_FILE
	done
	${ECHO} "\"npath\"=${j}" >> $WINDOWS_FILE
# print lines to delete existing path flags
	${ECHO} "; delete all existing path flags" >> $WINDOWS_FILE
	while [ $j -lt 100 ]; do
		((++j)) 
		${ECHO} "\"path${j}\"=-" >> $WINDOWS_FILE
	done
}

print_windows_nloadlib ()
{
	 ${ECHO} "\"nloadlib\"=${1}" >> $WINDOWS_FILE
	 ${ECHO} "Root: HKLM; SubKey: SOFTWARE\Pd-extended; ValueType: string; ValueName: nloadlib; ValueData: ${1}; Tasks: libs; Flags: uninsdeletekey" >> $WINDOWS_INNO_REG_FILE
}
#==============================================================================#

${ECHO} "Running for GNU/Linux and Darwin:"
${ECHO} -e $GNULINUX_HEADER > $GNULINUX_FILE
${ECHO} -e $MACOSX_HEADER > $MACOSX_FILE
i=0
IFS=' '
for lib in $LIBS; do
	 ((++i)) 
	 ${ECHO} -n "$lib "
	 print_gnulinux $i $lib
	 print_macosx $i $lib
done
${ECHO} " "

print_gnulinux_nloadlib $i
print_macosx_nloadlib $i

# run separately so some libs can be excluded on Windows
${ECHO} "Running for Windows:"

${ECHO} -e $WINDOWS_HEADER > $WINDOWS_FILE
${ECHO} -e $WINDOWS_INNO_HEADER > $WINDOWS_INNO_REG_FILE

print_windows_helppath

i=0
IFS=' '
for lib in $LIBS; do
	 case "$lib" in
		  pdp) ${ECHO} -n "(ignoring $lib on Windows) " ;;
		  pidip) ${ECHO} -n "(ignoring $lib on Windows) " ;;
		  *)
				${ECHO} -n "$lib "
				((++i)) 
				print_windows $i $lib
				;;
	 esac
done
${ECHO} " "
print_windows_nloadlib $i

# print lines to delete existing loadlib flags
${ECHO} "; delete any previous loadlib flags" >> $WINDOWS_FILE
while [ $i -lt 100 ]; do
	 ((++i)) 
	 print_windows_delete loadlib $i
done

print_windows_reg_path
print_windows_inno_path
#
TMPFILE=$WINDOWS_INNO_FILE.`date +%s`
head -`grep -n "STARTHERE" $WINDOWS_INNO_FILE | cut -d ':' -f 1` $WINDOWS_INNO_FILE > $TMPFILE
cat $WINDOWS_INNO_REG_FILE >> $TMPFILE
FILE_LENGTH=`wc -l $WINDOWS_INNO_FILE | cut -d ' ' -f 1`
END_LENGTH=`grep -n "ENDHERE" $WINDOWS_INNO_FILE | cut -d ':' -f 1`
tail -`expr $FILE_LENGTH - $END_LENGTH + 1` $WINDOWS_INNO_FILE >> $TMPFILE
mv -f -- $TMPFILE $WINDOWS_INNO_FILE

# the .pd-settings file needs an end tag for the path statements
print_gnulinux_fontpath

print_macosx_fontpath
${ECHO} -e $MACOSX_FOOTER >> $MACOSX_FILE

