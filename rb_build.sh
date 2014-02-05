#!/bin/bash

. ./rb_core.sh

# Needed tools
InstallPacket 'wget'
InstallPacket 'gperf'
InstallPacket 'bison'
InstallPacket 'makeinfo'
InstallPacket 'pkg-config'
InstallPacket 'subversion'

# Directories
CreateDirIfNotExists "${TMP_DIR}"
CreateDirIfNotExists "${LOG_DIR}"
CreateDirIfNotExists "${BUILD_DIR}"
CreateDirIfNotExists "${BOARD_DIR}"
CreateDirIfNotExists "${CONF_DIR}"
CreateDirIfNotExists "${OUTPUT_DIR}"
CreateDirIfNotExists "${UBOOT_IMG_DIR}"
CreateDirIfNotExists "${KERNEL_IMG_DIR}"
CreateDirIfNotExists "${ROOTFS_IMG_DIR}"
CreateDirIfNotExists "${TOOLCHAIN_DIR}"
CreateDirIfNotExists "${BUILDROOT_DIR}"
CreateDirIfNotExists "${PACKETS_DIR}"
CreateDirIfNotExists "${INCLUDE_DIR}"
BindOrCreateDir "${DOWNLOAD_DIR}" "${EXT_DOWNLOAD_DIR}"

# Default values
BOARD_NAME='pc'
BOARD_ARCH='x86'
BOARD_CPU='x86'
BOARD_PREFIX='unknown'
BOARD_CTNG_VER='1.16.0'
SUBPROG_TYPE="$1"
SUBPROG_ARG="$2"
SUBPROG_ARG1="$3"

# Loading board configuration
LoadBoardConfig ${CORE_SELECT_BOARD}
# Configurating (setting of special environment variables)
ConfigurateToolchain
ConfigurateUBoot
ConfigurateKernel
ConfigurateRootFS
ConfigurateInitRAMFS
# Method for canceling of not finished operations
CancelKernel
CancelRootFS

UploadImages() {
	UBootToRemoteRepo
	KernelToRemoteRepo
	RootFSToRemoteRepo
}

BootOverUSB() {
	local imx_usb_loader_path="${SRC_UTILS_DIR}/imx-usb-loader/"
	local imx_usb_tool="$imx_usb_loader_path/imx-usb-loader"
	local uboot_img="$imx_usb_loader_path/u-boot.imx"
	IsFileExists $imx_usb_tool || PrintAndDie "iMX USB loader not found!"
	IsFileExists $uboot_img || PrintAndDie "u-boot image not found!"
	FindUSBDevice 'Freescale' || PrintAndDie "iMX board not found on USB bus!"
	PrintNotice "Sending image to board..."
	sudo $imx_usb_tool $uboot_img
}

# Run subprogram
case "${SUBPROG_TYPE}" in
	# Printing help
	'help'      ) PrintHelp;;
	# Select board config
	'board'     ) SelectBoardConfig;;
	# Building toolchain
	'toolchain' ) BuildToolchain;;
	# Building u-boot
	'uboot'     ) BuildUBoot;;
	# Building kernel
	'kernel'    ) BuildKernel;;
	# Building rootfs
	'rootfs'    ) BuildRootFS;;
	# Building initramfs
	'initramfs' ) BuildInitRAMFS;;
	# Building external applications and libraries
	'packets'   ) BuidPackets;;
	# Creating rootfs image
	'image'     );;
	# Prepare MMC
	'mmc'       ) BuildMMC;;
	# Upload images and misc to remote repository
	'upload'    ) UploadImages;;
	# Boot board over USB
	'usb-boot'  ) BootOverUSB;;
	# default
	*           ) PrintWarn "Unknown subprogram: ${SUBPROG_TYPE}";;
esac
