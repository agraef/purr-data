#!/bin/sh
cd ../
mv Gem/src/.svn svn
echo "recursively removing .svn folders from"
pwd
rm -rf `find . -type d -name .svn`
mv svn Gem/src/.svn
exit 0
