#!/bin/sh
. /usr/lib/regisetup/def.sh
# Sysfs interfaces
SYS_GPIO='/sys/class/gpio'
SYS_GRAPHIC='/sys/class/graphics'
# System info
BOARD_NAME=$(cat /proc/cpuinfo | grep -E 'Hardware(.*):(.*)' | sed -r 's/(.+): (.+)/\2/g') #'
HW_ADDR=$(cat /etc/hwaddr)

IsItMX53

ReadFuse() {
	ThrowUndefined $FUSE_DEV 'Name of FUSE device!'
	local fuse_addr=`echo $(($2))`
	local fuse_val=$(dd if=$FUSE_DEV bs=1 skip=$fuse_addr count=1 2> /dev/null | \
	  hexdump | \
	  head -1 | \
	  sed -r 's/^\w+\ ([0-9a-f]{4,4})/\1/g')
	Notice "FUSE [$2] $1: $fuse_val"
	local fuse_parser=$3
	IsDefined $fuse_parser || return
	$fuse_parser $fuse_val
}
