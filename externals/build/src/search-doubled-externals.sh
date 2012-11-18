#!/bin/sh
#
# run this script to see doubled externals in the
# libdir and flatspace format
#
# these files can be removed from the flatspace
#
# (Georg Holzmann)

# all files
FILES=`ls *.c | sed s/"\.c"/"\.p\*"/g`

# search dir
# (should point to the directory where you build pd-extended)
SEARCH_PATH=/usr/local/lib/pd-extended/extra

for FILE in $FILES
do

  # this will also display the path (for debugging):
  # find $SEARCH_PATH -name $FILE

  # this will display the files, which can be deleted:
  find $SEARCH_PATH -name $FILE | sed -e s/"\.\."/""/g -e s/"\/.*\/"/""/g -e s/"\.pd_linux"/"\.c"/g

done

