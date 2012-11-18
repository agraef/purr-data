#!/bin/sh

rm images.txt
touch images.txt
for file in *.gif; do 
    name=`echo $file | sed 's|\.gif||'`
    echo $name
    echo "set ${name}data {" >> images.txt
    uuencode -m $name.gif temp | tail -n +2 | head -n -1 >> images.txt
    echo "}" >> images.txt 
    echo "image create photo buttonimage$name -data \$${name}data" >> images.txt 
done
