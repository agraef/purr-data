#!/bin/sh


# the source dir where this script is
SCRIPT_DIR=$(echo $0 | sed 's|\(.*\)/.*$|\1|')
. $SCRIPT_DIR/auto-build-common

# the name of this script
SCRIPT=$(echo $0| sed 's|.*/\(.*\)|\1|g')

mailbody_on_failure () {
 ## this is a somewhat simplistic expression to detect error-lines
 cat "$1" | grep -i "error: " | tail -20
 echo ""
 echo "the full logfile can be viewed at"
 echo "http://autobuild.puredata.info/auto-build/${DATE}/logs/${LOGFILE##*/}"
}


for logfile in $(ls -1 /var/www/auto-build/${DATE}/logs/${DATE}_*_mingw*.txt); do
	 completion_test=$(tail -1 "${logfile}")
	 if [ "x${completion_test}" != "xSUCCESS" ]; then
		if [ "x${RECIPIENT}" != "x" ]; then
		  SUBJECT="autobuild: $distro $HOSTNAME $DATE $TIME"
		  mailbody_on_failure "${logfile}" | mail -s "autobuild: $logfile" ${RECIPIENT}
		fi
	 fi
done
