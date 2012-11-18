#!/bin/bash

cd ../externals



# toxy
make toxy

cp miXed/bin/tot.pd_linux build/l2ork/toxy/
cp miXed/bin/tow.pd_linux build/l2ork/toxy/
cp miXed/bin/widget.pd_linux build/l2ork/toxy/

cp miXed/doc/help/toxy/* build/l2ork/toxy/

echo "#N canvas 10 10 200 200 10;
#N canvas 20 20 420 300 META 0;
#X text 10 10 META this is a prototype of a libdir meta file;
#X text 10 10 NAME toxy;
#X text 10 10 AUTHOR Kzrysztof Czaja;
#X text 10 10 LICENSE BSD;
#X text 10 10 DESCRIPTION objects for working with Tcl and Pd's Tk GUI;
#X restore 10 10 pd META;" > build/l2ork/toxy/toxy-meta.pd



# flib
make flib

cp postlude/flib/src/*.pd_linux build/l2ork/flib/
cp postlude/flib/doc/* build/l2ork/flib/

echo "#N canvas 10 10 200 200 10;
#N canvas 20 20 420 300 META 0;
#X text 10 10 META this is a prototype of a libdir meta file;
#X text 10 10 NAME flib;
#X text 10 10 AUTHOR Jamie Bullock;
#X text 10 10 DESCRIPTION library for feature extraction;
#X text 10 10 LICENSE GNU GPL;
#X restore 10 10 pd META;" > build/l2ork/flib/flib-meta.pd



# flatspace
make flatspace

cp build/src/*pd_linux build/l2ork/flatspace/



# libdir
make loaders-libdir
cp loaders/libdir/libdir.pd_linux build/l2ork/

exit 0

