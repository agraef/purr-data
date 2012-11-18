#!/bin/sh

# this script builds the apt repo for http://apt.puredata.info/releases

cd /var/www/releases

# delete old ones
find dists -name Packages.gz -delete
find dists -name Packages.bz2 -delete

# make new Packages files
for dir in dists/*/*/binary-*; do
	 dpkg-scanpackages ${dir} /dev/null | gzip -9c > ${dir}/Packages.gz
	 dpkg-scanpackages ${dir} /dev/null | bzip2 -9c > ${dir}/Packages.bz2
done

for dir in dists/*; do
	 apt-ftparchive contents $dir | gzip -9c > ${dir}/Contents.gz
	 apt-ftparchive release $dir > ${dir}/Release
done


