#!/bin/sh


PD_ROOT=$1

NETRECEIVE_PATCH=/tmp/.____pd_netreceive____.pd
PORT_NUMBER=55556
LOG_FILE=/tmp/load_every_object-log-`date +20%y-%m-%d_%H.%M.%S`.txt

TEST_DIR=/tmp
TEST_PATCH=.____test_patch.pd

PRE_SLEEP=30

helpdir=${PD_ROOT}/doc
objectsdir=${PD_ROOT}/extra
bindir=${PD_ROOT}/bin

#PDSEND=${bindir}/pdsend
#PD=${bindir}/pd

# if ${PD} is not set, it will be set to ${bindir}/pd
# if ${PD} is already set, this value will be used
## this way we can use a 'default' pd to test externals
## that live not necessarily in ${PD_ROOT}/bin/../extra
if test "x${PD}" = "x" ; then
 PD=${bindir}/pd
fi
if test "x${PDSEND}" = "x" ; then
 PDSEND=${PDSEND:-${bindir}/pdsend}
fi

# just imagine there is no [...]/extra directory
# because i want to test a bunch of "not installed"
# objects...
if test -d "${objectsdir}" ; then
 :
else
 objectsdir=${PD_ROOT}
fi


make_netreceive_patch () 
{
	 rm $1
	 touch $1
	 echo '#N canvas 222 130 454 304 10;' >> $1
	 echo "#X obj 111 83 netreceive $PORT_NUMBER 0 old;" >> $1
}

make_patch ()
{
	 rm $2
	 touch $2
	 object=`echo $1|sed 's|^\(.*\)\.[adilnpruwx_]*$|\1|'`
	 echo '#N canvas 222 130 454 304 10;' >> $2
	 echo "#X obj 111 83 $object;" >> $2
}

open_patch ()
{
	 echo "OPENING: $1 $2" >> $LOG_FILE
	 echo "; pd open $1 $2;" | ${PDSEND} $PORT_NUMBER localhost tcp
}

close_patch ()
{
	 echo "CLOSING: $1" >> $LOG_FILE
	 echo "; pd-$1 menuclose;" | ${PDSEND} $PORT_NUMBER localhost tcp
}

UNAME=`uname -s`
if test "x$UNAME" == "xDarwin" ; then
	 EXTENSION=pd_darwin
elif test "x$UNAME" == "xLinux" ; then
	 EXTENSION=pd_linux
else
	 EXTENSION=dll
fi

echo "Searching for ${EXTENSION} in ${objectsdir}"

make_netreceive_patch $NETRECEIVE_PATCH

touch $LOG_FILE
${PD} -nogui -stderr -open $NETRECEIVE_PATCH -path ${objectsdir} >> $LOG_FILE 2>&1 &

#wait for pd to start
echo -n going to sleep for ${PRE_SLEEP} secs...
sleep ${PRE_SLEEP}
echo "ready to perform!"

#for file in `find $objectsdir -name "*.${EXTENSION}"`; do
find ${objectsdir} -name "*.${EXTENSION}" | while read file; do
	 echo $file
	 #JMZ: wow, couldn't this be done with following?
	 # filename=${file##*/}
	 # dir=${file%/*}
	 filename=`echo $file|sed 's|.*/\(.*\.[adilnpruwx_]*\)$|\1|'`
	 dir=`echo $file|sed 's|\(.*\)/.*\.[adilnpruwx_]*$|\1|'`
	 make_patch $filename ${TEST_DIR}/${TEST_PATCH}
	 open_patch ${TEST_PATCH} ${TEST_DIR}
	 sleep 1
	 close_patch ${TEST_PATCH}
done


echo "COMPLETED!" >> $LOG_FILE
echo "; pd quit;" | ${PDSEND} $PORT_NUMBER localhost tcp
