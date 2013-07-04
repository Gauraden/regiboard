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

# Run subprogram
case "${SUBPROG_TYPE}" in
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
	# default
	*           ) PrintWarn "Unknown subprogram: ${SUBPROG_TYPE}";;
esac
