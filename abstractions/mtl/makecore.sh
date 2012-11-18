#!/bin/sh
echo "Creating core tags"
rm core.txt
grep "tags:" *-help.pd | sed 's/-help.*tags://' | sed 's/^/mtl\//'| sed 's/;//' > core.txt

echo "Sorting love"
sort -o amourette.txt love.txt
rm love.txt
mv amourette.txt love.txt

echo "Making copy of browser.pd"
cp browser.pd 1.mtlBrowser.pd
