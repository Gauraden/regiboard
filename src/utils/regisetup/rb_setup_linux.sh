#!/bin/sh

. /usr/lib/regisetup/hw.sh

PATH2IMAGE=''

# You can use this commands to detect id of UBI device
#cat /sys/class/misc/ubi_ctrl/dev
#cat /sys/class/ubi/ubi0/dev

FindImage() {
	ThrowUndefined "$1" 'Name of image'
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
	  Throw "Image '$1' was not found, installation could not continue"
}

SetupMAC() {
  Notice 'Generating of MAC...'
  local HW_ADDR=$(printf '00:60:2F:%02X:%02X:%02X' $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256)))
  Print "Addr: ${HW_ADDR}"
  echo "$HW_ADDR" > ${1}/etc/hwaddr
}

PreparingUBIFS() {
	Print 'Preparing UBI file system...'
	ThrowUndefined "$2" 'UBI volume size'
	ThrowUndefined "$3" 'MTD name: cat /proc/mtd'
	local vol_name="$3"
	local mtd_dev=$(GetMTDFor "$vol_name") #"mtd$1"
	local mtd_num=$(echo "$mtd_dev" | grep -E -o '([0-9]+)') #'
	local ubi_dev="ubi$1"
	local vol_size="$2"
	if ! IsFileExists '/dev/ubi_ctrl'; then
		Notice 'Creating "ubi_ctrl" device'
		mknod /dev/ubi_ctrl c 10 63
	else
		ubidetach /dev/ubi_ctrl -m $mtd_num
	fi
	Notice "Formating \"${mtd_dev}\" to UBI format"
	ubiformat /dev/${mtd_dev}
	Notice "Creating UBI volume (${vol_size}): "
	ubiattach /dev/ubi_ctrl -m $mtd_num
	ubimkvol  /dev/${ubi_dev} -N ${vol_name} -s ${vol_size} || \
	  Throw "Check params: /dev/${ubi_dev} -N ${vol_name} -s ${vol_size}"
}

SetupRootFS() {
	Print 'Setup root file system...'
	ThrowUndefined "$1" 'UBI device number'
	if ! IsFileExists "$2"; then
		GetFileFromFTP 'rootfs.tar' "$2" || Throw "Root FS image \"$2\" was not found!"
	fi
	if ! IsFileExists "$3"; then
		Throw "Mount point \"$3\" was not found!"
	fi
	Notice "Mounting of UBI$1 partition to: $3"
	mount -t ubifs ubi$1:rootfs "$3" || Throw "Check UBI device: ubi$1"
	Notice "Unpacking root FS image: $2"
	tar xvf "$2" -C "$3/" > ${_DEV_NULL}
	Notice 'Preparing home directory for "Regigraf" software'
	mkdir "$3/home/regigraf"
	echo 'ubi1:storage     /home/regigraf ubifs    defaults          0      0' >> "$3/etc/fstab"
	SetupMAC "$3"
}

InstallRootFS() {
  local vol_size=${UBI_ROOTFS_SIZE}
  if [ "$1" != "" ]; then
    vol_size=$1
  fi
  FindImage 'rootfs.tar'
	PreparingUBIFS 0 "$vol_size" "rootfs"
	SetupRootFS    0 "$PATH2IMAGE" '/mnt'
	unset PATH2IMAGE
}

InstallStorageFS() {
  local vol_size=${UBI_STORAGE_SIZE}
  if [ "$1" != "" ]; then
    vol_size=$1
  fi
	PreparingUBIFS 1 "$vol_size" "storage"
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
		Notice "Writing u-boot config to: $mtd..."
		flashcp -v "$uboot_conf" /dev/$mtd
	fi
	unset PATH2IMAGE
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
	unset PATH2IMAGE
}

DoAllOperations() {
	InstallBootLoader
	InstallRootFS $1
	InstallStorageFS $2
	InstallOS
}

Update() {
	local imgs_dir="$(pwd)"
	Notice 'Updating images...'
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
	*        ) Warn "Unknown subprogram: $1";;
esac

sync

