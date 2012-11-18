#!/bin/sh
if [ $# -ne 2 ]
then
echo "Usage: $0 original_name new_name"
exit
fi
cp ../pdmtl/$1-help.pd ./$2-help.pd \
&& cp ../pdmtl/$1.pd ./$2.pd \
&& svn add  ./$2-help.pd ./$2.pd
