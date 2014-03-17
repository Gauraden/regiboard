#!/bin/sh

PKG_CONF_GLIB_FLG=`pkg-config --cflags --libs glib-2.0`
PKG_CONF_GTK_FLG=`pkg-config --cflags gtk+-2.0 webkit-1.0`
PKG_CONF_GTK_LIB=`pkg-config --libs gtk+-2.0 webkit-1.0`
LIBSOUP_INC="/usr/include/libsoup-2.4"
COMPILER_FLAGS="-O2 -Wall -I/usr/include -I${LIBSOUP_INC} ${PKG_CONF_GLIB_FLG} ${PKG_CONF_GTK_FLG}"
LINKER_FLAGS="-L/usr/lib -lc ${PKG_CONF_GTK_LIB}"

cd ./build/ && 
export LDFLAGS="${LINKER_FLAGS}" CFLAGS="${COMPILER_FLAGS}" CXXFLAGS="${COMPILER_FLAGS}" &&
cmake ../ && make install
