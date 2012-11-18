#!/bin/sh  

SYS=pd-linux

. config-${SYS}.txt

make -f makefile.${SYS} &&
{ 
	if [ $INSTDIR != "" ]; then
		echo Now install as root
		su -c "make -f makefile.${SYS} install"
	fi
	if [ $HELPDIR != "" ]; then
		echo Now install help as root
		su -c "make -f makefile.${SYS} install-help"
	fi
}
