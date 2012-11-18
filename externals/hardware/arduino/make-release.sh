#!/bin/sh

if [ $# -ne 1 ]; then
    echo "Usage: $0 FIRMATA_VERSION"
    exit
fi

PDUINO_VERSION=`grep version_ arduino.pd | sed 's|.*version_\(.*\);|\1|'`
FIRMATA_VERSION=$2

PDUINO_DIR=Pduino-${PDUINO_VERSION}
FIRMATA_DIR=Firmata-${FIRMATA_VERSION}

mkdir /tmp/pduino-release
cd /tmp/pduino-release
svn co https://pure-data.svn.sourceforge.net/svnroot/pure-data/trunk/externals/hardware/arduino $PDUINO_DIR

cd $PDUINO_DIR
/sw/bin/svn2cl
svn co https://firmata.svn.sourceforge.net/svnroot/firmata/arduino/trunk $FIRMATA_DIR

cd $FIRMATA_DIR
/sw/bin/svn2cl
cd ..

#remove cruft
find . -name .DS_Store -delete
find . -name .svn -print0 | xargs -0 rm -r
rm -rf PICduino examples arduino-stress-test.pd

zip -9r ../${FIRMATA_DIR}.zip $FIRMATA_DIR
cd ..
zip -9r ${PDUINO_DIR}.zip $PDUINO_DIR

# add to CVS for my website
cp -a ${FIRMATA_DIR}.zip ${PDUINO_DIR}.zip ~/code/works/pd/
cd ~/code/works/pd
cvs add ${FIRMATA_DIR}.zip ${PDUINO_DIR}.zip

# change the versions in the HTML
sed -i "s|Firmata-[0-9][.0-9A-Za-z_-]*\.zip|${FIRMATA_DIR}.zip|g" objects.html
sed -i "s|Pduino-[0-9][.0-9A-Za-z_-]*\.zip|${PDUINO_DIR}.zip|g" objects.html





