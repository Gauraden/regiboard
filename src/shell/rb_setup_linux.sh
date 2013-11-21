#!/bin/sh

. "$(pwd)/rb_functions.sh"

UBI_VOL0_SIZE='120MiB'
UBI_VOL1_SIZE='350MiB'
PATH2IMAGE=''

FTP_REPO_URL='regiboard@jenny'
FTP_REPO_PSW='12345'
# You can use this commands to detect id of UBI device
#cat /sys/class/misc/ubi_ctrl/dev
#cat /sys/class/ubi/ubi0/dev

PreparingUBIFS() {
	Print 'Preparing UBI file system...'
	DieIfNotDefined "$2" 'UBI volume size'
	DieIfNotDefined "$3" 'MTD name: cat /proc/mtd'
	local mtd_dev=$(GetMTDFor "$3") #"mtd$1"
	local mtd_num=$(echo "$mtd_dev" | grep -E -o '([0-9]+)') #'
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
	ubimkvol  /dev/${ubi_dev} -N nandfs -s ${vol_size} || PrintAndDie "Check size: ${vol_size}!"
}

SetupRootFS() {
	Print 'Setup root file system...'
	DieIfNotDefined "$1" 'UBI device number'
	if ! IsFileExists "$2"; then
		GetFileFromFTP 'rootfs.tar' "$2" || PrintAndDie "Root FS image \"$2\" was not found!"
	fi
	if ! IsFileExists "$3"; then
		PrintAndDie "Mount point \"$3\" was not found!"
	fi
	PrintNotice "Mounting of UBI$1 partition to: $3"
	mount -t ubifs ubi$1:nandfs "$3" || PrintAndDie "Check UBI device: ubi$1"
	PrintNotice "Unpacking root FS image: $2"
	tar xvf "$2" -C "$3/" > ${_DEV_NULL}
	PrintNotice 'Preparing home directory for "Regigraf" software'
	mkdir "$3/home/regigraf"
	echo 'ubi1:nandfs     /home/regigraf ubifs    defaults          0      0' >> "$3/etc/fstab"
}

InstallRootFS() {
  local vol_size=${UBI_VOL0_SIZE}
  if [ "$1" != "" ]; then
    vol_size=$1
  fi
	PreparingUBIFS 0 "$vol_size" "rootfs"
	SetupRootFS    0 "$(pwd)/rootfs.tar" '/mnt'
}

InstallStorageFS() {
  local vol_size=${UBI_VOL1_SIZE}
  if [ "$1" != "" ]; then
    vol_size=$1
  fi
	PreparingUBIFS 1 "$vol_size" "storage"
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
	GetFileFromFTP "$1" "./" && export PATH2IMAGE="./$1" || \
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
	InstallRootFS $1
	InstallStorageFS $2
	InstallOS
}

Update() {
	local imgs_dir="$(pwd)"
	PrintNotice 'Updating images...'
	rm "$imgs_dir/uImage"
	rm "$imgs_dir/rootfs.tar"
	GetFileFromFTP 'uImage'     "$imgs_dir/"
	GetFileFromFTP 'rootfs.tar' "$imgs_dir/"
}

# Run subprogram
case "$1" in
	# Installing root fs
	'rootfs' ) InstallRootFS $2;;
	# Installing storage fs
	'storage') InstallStorageFS $2;;
	# Installing bootloader
	'boot'   ) InstallBootLoader;;
	# Installing OS
	'os'     ) InstallOS;;
	# Do all operations
	'all'    ) DoAllOperations $2 $3;;
	# Update all images and scripts
	'update' ) Update;;
	# default
	*        ) PrintWarn "Unknown subprogram: $1";;
esac

sync

