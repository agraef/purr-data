curdir=`pwd`
git clone git://git.drogon.net/wiringPi
cd wiringPi/ && git pull && cd wiringPi/ && make static
cd $curdir
make
exit 0
