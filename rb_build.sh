#!/bin/bash

. ./rb_core.sh

# Needed tools
InstallPacket 'wget'
InstallPacket 'gperf'
InstallPacket 'bison'
InstallPacket 'makeinfo'
InstallPacket 'pkg-config'
InstallPacket 'subversion'
InstallPacket 'flex'
InstallPacket 'texinfo'
InstallPacket 'gawk'

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
CreateDirIfNotExists "${FIRMWARE_DIR}"
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

if [ "${SUBPROG_TYPE}" == 'board' ]; then
  SelectBoardConfig $SUBPROG_ARG
fi

if IsFileExists "${CORE_SELECT_CONF}"; then
	Print "Predefined settings:"
	. "${CORE_SELECT_CONF}"
	IsDefined ${CORE_SELECT_BOARD} ||	SelectBoardConfig
	PrintNotice "Board: ${CORE_SELECT_BOARD}"
else
	SelectBoardConfig
fi

# Checking packets sources for update
CheckSourcesOfPackets
# Loading board configuration
LoadBoardConfig ${CORE_SELECT_BOARD} || SelectBoardConfig
# Configurating (setting of special environment variables)
ConfigurateToolchain
ConfigurateUBoot
ConfigurateKernel
ConfigurateRootFS
ConfigurateInitRAMFS
ConfigurateFirmware
# Method for canceling of not finished operations
CancelKernel
CancelRootFS

UploadImages() {
	UBootToRemoteRepo
	KernelToRemoteRepo
	RootFSToRemoteRepo
}

BootOverUSB() {
	local imx_usb_tool="${BIN_DIR}/imx-usb-loader"
	local uboot_img="${UBOOT_IMG_DIR}/u-boot.${TARGET_NAME_CPU}.imx"
	IsFileExists $imx_usb_tool || ./rb_build.sh packets imx-usb-loader
	IsFileExists $uboot_img || ConvertBinToImx ${UBOOT_IMG_DIR}
	FindUSBDevice 'Freescale' || PrintAndDie "iMX board not found on USB bus!"
	PrintNotice "Sending image to board..."
	sudo $imx_usb_tool $uboot_img
}

StartOpenOCD() {
  local openocd="${BIN_DIR}/openocd"
  local cfg_dir="${CONF_DIR}/openocd"
  local log_file="${LOG_DIR}/openocd"
  IsFileExists $openocd || ./rb_build.sh packets openocd
  PrintNotice "Starting openOCD server..."
  $openocd -f $cfg_dir/olimex-arm-usb-ocd.cfg -f $cfg_dir/imx53.cfg \
     > ${log_file}.server.log \
    2> ${log_file}.error.log &
}

ConnectToOpenOCD() {
  local dbg_pid=$(pidof openocd)
  IsDefined $dbg_pid || StartOpenOCD
  PrintNotice "Connecting to openOCD server..."
  telnet localhost 4444
}

# Run subprogram
case "${SUBPROG_TYPE}" in
	# Printing help
	'help'      ) PrintHelp;;
	# Select board config
	'board'     ) SelectBoardConfig $SUBPROG_ARG;;
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
	# Building firmware
	'firmware'  ) BuildFirmware;;
	# Creating rootfs image
	'image'     );;
	# Prepare MMC
	'mmc'       ) BuildMMC;;
	# Upload images and misc to remote repository
	'upload'    ) UploadImages;;
	# Boot board over USB
	'usb-boot'  ) BootOverUSB;;
	# Starting openOCD server and/or client
	'debug'     ) ConnectToOpenOCD;;
	# default
	*           ) PrintWarn "Unknown subprogram: ${SUBPROG_TYPE}"; PrintHelp;;
esac
