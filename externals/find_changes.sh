#!/bin/bash
# source diff for externals folder by Ivica Ico Bukvic <ico@vt.edu> (c) 2012
# released under GPL v3 license

usage()
{
	echo
	echo "   Usage: ./find_changes.sh -option file_extension"
	echo "   Options:"
	echo "     -d    select different"
	echo "     -r    replace different"
	echo "     -v    show differences"
	echo "     -x    select those that do not exist"
	echo
	exit 1
}

if [ $# -eq 0 ] # should check for no arguments
then
	usage
fi

different=0
exist=0
replace=0
show=0
#n=0
d=""

while getopts ":drvx" Option
do case $Option in
		d)		different=1;;

		r)		replace=1;;

		v)		show=1;;

		x)		exist=1;;

		*)		echo "Error: unknown option"
				usage;;
	esac
done

list=`find ./ -name ${!#}`

for i in $list
do
	#echo $n:$i
	#((n++))
	if [ ! -f ~/Downloads/PureData/pure-data/externals/$i ]
	then
		if [ $exist -eq 1 ]
		then
			echo DOES_NOT_EXIST: $i
		fi
	else
		d=`diff -u ~/Downloads/PureData/pure-data/externals/$i $i`
		if [ "$d" != "" ]
		then
			if [ $different -eq 1 ]
			then
				echo DIFFERENT: $i
				if [ $show -eq 1 ]
				then
					echo "$d"				
				fi
			fi
			if [ $replace -eq 1 ]
			then
				echo REPLACE
				cp -f $i ~/Downloads/PureData/pure-data/externals/$i
			fi
		fi
	fi
done

exit 0
