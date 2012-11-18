#!/bin/sh  

. config-pd-linux.txt

make -f makefile.pd-linux &&
{ 
	if [ $INSTDIR != "" ]; then
		echo Now install as root
		su -c "make -f makefile.pd-linux install"
	fi
}
