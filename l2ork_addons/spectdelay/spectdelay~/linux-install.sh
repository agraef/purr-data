cd ../genlib
rm -f *.o
gcc -lm -c -fPIC -o Offt.o Offt.cpp
gcc -c -fPIC -o Ooscil.o Ooscil.cpp
gcc -c -fPIC -o RandGen.o RandGen.cpp
gcc -c -fPIC -o FFTReal.o FFTReal.cpp
gcc -c -fPIC -o Obucket.o Obucket.cpp
gcc -c -fPIC -o Odelay.o Odelay.cpp
cd ../Spectacle
rm -f *.o
gcc -c -fPIC -o SpectacleBase.o SpectacleBase.cpp
gcc -c -fPIC -o SpectEQ.o SpectEQ.cpp
gcc -c -fPIC -o Spectacle.o Spectacle.cpp
cd ../spectdelay~
rm -f *.o
gcc -DPD -g -fPIC -I/usr/local/include/pdl2ork -I../Spectacle -o spectdelay~.o -c spectdelay~.cpp
gcc -Wall -W -Wstrict-prototypes -Wno-unused -Wno-parentheses -Wno-switch -O6 -funroll-loops -fomit-frame-pointer -fno-strict-aliasing -fPIC -DUNIX -Wl,--export-dynamic -shared ../Spectacle/Spectacle.o ../Spectacle/SpectacleBase.o  ../genlib/Odelay.o ../genlib/Obucket.o ../genlib/FFTReal.o ../genlib/Offt.o -o spectdelay~.pd_linux spectdelay~.o
strip --strip-unneeded *.pd_linux
