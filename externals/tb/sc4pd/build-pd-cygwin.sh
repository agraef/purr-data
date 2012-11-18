#!/bin/sh  

SYS=pd-cygwin

. config-${SYS}.txt

make -f makefile.${SYS} &&
{ 
	if [ $INSTDIR != "" ]; then
		make -f makefile.${SYS} install
	fi
	if [ $HELPDIR != "" ]; then
		make -f makefile.${SYS} install-help
	fi
}
