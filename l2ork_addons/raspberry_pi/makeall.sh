git clone https://github.com/WiringPi/WiringPi.git wiringPi
cd wiringPi
git stash
git pull
cd wiringPi
make static
cd ../../disis_gpio
./build.sh
cd ../

cd disis_spi
make
cd ../

exit 0
