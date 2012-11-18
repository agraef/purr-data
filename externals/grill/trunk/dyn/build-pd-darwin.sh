#!/bin/sh  

. config-pd-darwin.txt

make -f makefile.pd-darwin &&
{ 
	if [ $INSTDIR != "" ]; then
		echo Now install as root
		sudo make -f makefile.pd-darwin install
	fi
}
