#!/bin/sh

TOOLCHAIN_DIR="/home/denis/Projects/buildroot/output"
TOOLCHAIN_UCLIBC_EABI="${TOOLCHAIN_DIR}/host/usr/bin/arm-unknown-linux-uclibcgnueabi-"
TOOLCHAIN_ROOTFS="${TOOLCHAIN_DIR}/staging"

CXX_COMPILER="${TOOLCHAIN_UCLIBC_EABI}g++"
CPP_COMPILER="${TOOLCHAIN_UCLIBC_EABI}cpp"
C_COMPILER="${TOOLCHAIN_UCLIBC_EABI}gcc"
PKG_CONF_GLIB_FLG=`${TOOLCHAIN_DIR}/host/usr/bin/pkg-config --cflags --libs glib-2.0`
PKG_CONF_GTK_FLG=`${TOOLCHAIN_DIR}/host/usr/bin/pkg-config --cflags gtk+-directfb-2.0 webkit-1.0`
PKG_CONF_GTK_LIB=`${TOOLCHAIN_DIR}/host/usr/bin/pkg-config --libs gtk+-directfb-2.0 webkit-1.0`
LIBSOUP_INC="${TOOLCHAIN_ROOTFS}/usr/include/libsoup-2.4"
COMPILER_FLAGS="-O2 -Wall -g -I${TOOLCHAIN_ROOTFS}/usr/include -I${LIBSOUP_INC} ${PKG_CONF_GLIB_FLG} ${PKG_CONF_GTK_FLG}"
LINKER_FLAGS="-L${TOOLCHAIN_ROOTFS}/usr/lib ${PKG_CONF_GTK_LIB}"

cd ./build/ && 
export CC=${C_COMPILER} CXX=${CXX_COMPILER} CPP=${CPP_COMPILER} LDFLAGS="${LINKER_FLAGS}" CXXFLAGS="${COMPILER_FLAGS}" CFLAGS="${COMPILER_FLAGS}"&&
cmake ../ && make install
