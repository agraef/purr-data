#!/bin/bash

#set -x

while read line
do
  for word in $line
  do
    if [ "X"$word != "X"${word#\#} ]
    then
       echo // ${line#\#}
       break
    else
       line=${line//\'/\\\'}
#useless, slashes never gets in
       line=${line//\\/\\\\}
#this one's dirty, i know
       line=${line//;/\\\\;}
       line=${line//\"/\\\"}
       echo 'sys_gui("'$line'\n");'
       break
    fi
  done
done
