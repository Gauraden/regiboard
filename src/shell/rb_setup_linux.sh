#!/bin/sh

. "$(pwd)/rb_functions.sh"

UBI_VOL0_SIZE='248MiB'
PATH2IMAGE=''

# You can use this commands to detect id of UBI device
#cat /sys/class/misc/ubi_ctrl/dev
#cat /sys/class/ubi/ubi0/dev

PreparingUBIFS() {
	Print 'Preparing UBI file system...'
	DieIfNotDefined "$2" 'UBI volume size'
	local mtd_dev=$(GetMTDFor 'rootfs') #"mtd$1"
	local mtd_num=$(echo "$mtd_dev" | grep -E -o '([0-9]+)')
	local ubi_dev="ubi$1"
	local vol_size="$2"
	if ! IsFileExists '/dev/ubi_ctrl'; then
		PrintNotice 'Creating "ubi_ctrl" device'
		mknod /dev/ubi_ctrl c 10 63
	else
		ubidetach /dev/ubi_ctrl -m $mtd_num
	fi
	PrintNotice "Formating \"${mtd_dev}\" to UBI format"
	ubiformat /dev/${mtd_dev}
	PrintNotice "Creating UBI volume (${vol_size}): "
	ubiattach /dev/ubi_ctrl -m $mtd_num
	ubimkvol  /dev/${ubi_dev} -N nandfs -s ${vol_size}
}

SetupRootFS() {
	Print 'Setup root file system...'
	DieIfNotDefined "$1" 'UBI device number'
	if ! IsFileExists "$2"; then
		PrintAndDie "Root FS image \"$2\" was not found!"
	fi
	if ! IsFileExists "$3"; then
		PrintAndDie "Mount point \"$3\" was not found!"
	fi
	PrintNotice "Mounting of UBI$1 partition to: $3"
	mount -t ubifs ubi$1:nandfs "$3"
	PrintNotice "Unpacking root FS image: $2"
	tar xvf "$2" -C "$3/" > ${_DEV_NULL}
}

InstallRootFS() {
  local vol_size=${UBI_VOL0_SIZE}
  if [ "$1" != "" ]; then
    vol_size=$1
  fi
	PreparingUBIFS 0 "$vol_size"
	SetupRootFS    0 "$(pwd)/rootfs.tar" '/mnt'
}

FindImage() {
	DieIfNotDefined "$1" 'Name of image'
	# First of all let's check the boot partition of MMC
	if IsFileExists "/boot/$1"; then
		PATH2IMAGE="/boot/$1"
		return
	fi
	# At last checking local directory
	if IsFileExists "./$1"; then
		PATH2IMAGE="./$1"
		return
	fi
	PrintAndDie "Image '$1' was not found, installation could not continue"
}

InstallBootLoader() {
	FindImage 'u-boot.bin'
	local mtd=$1
	if [ "$mtd" = '' ]; then
		mtd=$(GetMTDFor 'bootloader')
	fi
	Print "Writing u-boot image '$PATH2IMAGE' to: $mtd ..."
	flashcp -v "$PATH2IMAGE" /dev/$mtd
	local uboot_conf='/boot/u-boot_nand_config.bin'
	if IsFileExists "$uboot_conf"; then
		mtd=$(GetMTDFor 'config')
		PrintNotice "Writing u-boot config to: $mtd..."
		flashcp -v "$uboot_conf" /dev/$mtd
	fi
}

InstallOS() {
	FindImage 'uImage'
	local mtd=$1
	if [ "$mtd" = '' ]; then
		mtd=$(GetMTDFor 'kernel')
	fi
	Print "Writing kernel image '$PATH2IMAGE' to: $mtd ..."
	mtd_debug erase /dev/$mtd 0 0x300000
	nandwrite -p /dev/$mtd "$PATH2IMAGE"
}

DoAllOperations() {
	InstallBootLoader
	InstallRootFS
	InstallOS
}

# Run subprogram
case "$1" in
	# Installing root fs
	'rootfs' ) InstallRootFS $2;;
	# Installing bootloader
	'boot'   ) InstallBootLoader;;
	# Installing OS
	'os'     ) InstallOS;;
	# Do all operations
	'all'    ) DoAllOperations;;
	# default
	*        ) PrintWarn "Unknown subprogram: $1";;
esac

sync

