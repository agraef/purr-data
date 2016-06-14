curdir=`pwd`
cd wiringPi/ && git stash && git pull && cd wiringPi/ && make static
cd $curdir
make
exit 0
