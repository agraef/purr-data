#!/bin/sh  

SYS=pd-darwin

. config-${SYS}.txt

make -f makefile.${SYS} &&
{ 
	if [ $INSTDIR != "" ]; then
		echo Now install as root
		sudo make -f makefile.${SYS} install
	fi
	if [ $HELPDIR != "" ]; then
		echo Now install help as root
		sudo make -f makefile.${SYS} install-help
	fi
}
