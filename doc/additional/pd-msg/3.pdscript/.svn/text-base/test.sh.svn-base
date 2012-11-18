#!/bin/sh

PORT=3005


function setfreq() 
{
pdsend $PORT <<EOF
freq $1 $2;
EOF
}

#
# Read in the script
#
./pdsend $PORT < test.txt

sleep 3
setfreq 220 5000
sleep 5
setfreq 1000 100
sleep 1
setfreq 100 50
sleep 1
setfreq 3000 1000
sleep 1
setfreq 100 1000
sleep 1
setfreq 3000 1000
sleep 1
setfreq 100 1000
sleep 1
setfreq 3000 1000
sleep 1
setfreq 100 1000
# and so on


