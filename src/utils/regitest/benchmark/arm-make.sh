#!/bin/bash

BASE="/opt/buildroot/build_arm/staging_dir"
#BASE="/home/denis/Проекты/buildroot/output/host/"
GXX="$BASE/usr/bin/arm-linux-g++ -fPIC"
LD="$BASE/usr/lib"
IN="$BASE/usr/include"

$GXX --sysroot=$BASE -o bm -L$LD -lz main.cpp
