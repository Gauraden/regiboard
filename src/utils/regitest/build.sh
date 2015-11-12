#!/bin/bash

## ------------------------------------------------------------------
FLAGS='--sysroot=/home/denis/Projects/regiboard/output/buildroot/buildroot.regigraf.cortex_a8/staging -I/home/denis/Projects/regiboard/output/include -I/home/denis/Projects/regiboard/output/buildroot/buildroot.regigraf.cortex_a8/staging/usr/include -I/home/denis/Projects/regiboard/output/buildroot/buildroot.regigraf.cortex_a8/staging/usr/include/directfb'
PATH='/home/denis/bin:/usr/local/bin:/usr/bin:/bin:/usr/games:/home/denis/Projects/regiboard/output/buildroot/buildroot.regigraf.cortex_a8/host/usr/bin'

export LDFLAGS='-Wl,-rpath-link,/home/denis/Projects/regiboard/output/buildroot/buildroot.regigraf.cortex_a8/target/usr/lib'
export CPPFLAGS="$FLAGS"
export CXXFLAGS="$FLAGS"
export CFLAGS="$FLAGS"

## ------------------------------------------------------------------

export CC=arm-cortex_a8-linux-gnueabi-gcc
export CXX=arm-cortex_a8-linux-gnueabi-g++
export CPP=arm-cortex_a8-linux-gnueabi-cpp

cd ./build && cmake ../ && make
cd ../

## ------------------------------------------------------------------

