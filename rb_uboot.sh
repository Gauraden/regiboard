#!/bin/sh

ConfigurateUBoot() {
	DieIfNotDefined ${BOARD_UBOOT_VER} "u-boot version"
	DieIfNotDefined ${BOARD_UBOOT_CNF} "u-boot configuration"
	UBOOT_IMG="u-boot.${TARGET_PREFIX}.bin"
	UBOOT_DIR="u-boot-${BOARD_UBOOT_VER}"
	UBOOT_BUILD_DIR="${BUILD_DIR}/${UBOOT_DIR}"
}

ConvertBinToImx() {
	PrintNotice "Creating u-boot.imx image"
	local output_dir=$1
	DieIfNotDefined ${output_dir} "output directory for *.imx image"
	IsFileExists ${output_dir} || (PrintErr "directory not found: ${output_dir}" && return)
	local mkimg="${BIN_DIR}/${TARGET_PREFIX}/mkimage"
	local nand_cfg="${CONF_DIR}/mkimage/imximage_nand.cfg"
	local sd_cfg="${CONF_DIR}/mkimage/imximage.cfg"
	local bin_img="${UBOOT_IMG_DIR}/${UBOOT_IMG}"
	local imx_img="${output_dir}/u-boot.${TARGET_PREFIX}"
#	$mkimg -n $nand_cfg -T imximage -e 0x77800000 -d $bin_img ${imx_img}.nand.imx
	$mkimg -n $sd_cfg -T imximage -e 0x77800000 -d $bin_img ${imx_img}.imx
}

BuildUBoot() {
	if ! IsFileExists ${UBOOT_BUILD_DIR}; then
		# Downloading
		# TODO
		# Unpacking
		UnpackArchive "${DOWNLOAD_DIR}/${UBOOT_DIR}.tar.bz2" "${BUILD_DIR}"
		ApplyAllPatchesFor "${UBOOT_DIR}" "${UBOOT_BUILD_DIR}"
	fi
	if [ "${SUBPROG_ARG}" = 'mkpatch' ]; then
		CreatePatch "${UBOOT_BUILD_DIR}"
		return 0
	fi
	if [ "${SUBPROG_ARG}" = 'config' ]; then
		DieIfNoTool 'mcedit'
		PrintNotice "Editing U-Boot config..."
		mcedit "${UBOOT_BUILD_DIR}/include/configs/${BOARD_UBOOT_CNF}.h"
		return 0
	fi
	PrintNotice "Clearing u-boot sources..."
	TcTargetDistCleanSources ${UBOOT_BUILD_DIR}
	PrintNotice "Configurating u-boot: ${BOARD_UBOOT_CNF}"
	TcTargetMakeSources ${UBOOT_BUILD_DIR} "${BOARD_UBOOT_CNF}_config"
	PrintNotice "Building u-boot.bin"
	TcTargetMakeSources ${UBOOT_BUILD_DIR} 'all'
	PrintNotice "Moving to output: ${UBOOT_IMG}"
	mv "${UBOOT_BUILD_DIR}/u-boot.bin" "${UBOOT_IMG_DIR}/${UBOOT_IMG}"
	ConvertBinToImx ${UBOOT_IMG_DIR}
	NotifyUser "Building of U-Boot \"${BOARD_UBOOT_CNF}\" was finished!"
}

UBootToMMC() {
	DieIfNoTool 'dd'
	DieIfNotDefined ${BOARD_UBOOT_DD} "dd options for writing u-boot image"
	DieIfNotDefined ${SUBPROG_ARG}    "mmc block device is undefined"
	if ! IsFileExists "${SUBPROG_ARG}"; then
		PrintAndDie "There is no device file: ${SUBPROG_ARG}"
	fi
	if [ ! -b "${SUBPROG_ARG}" ]; then
		PrintAndDie "It's not block device file: ${SUBPROG_ARG}"
	fi
	PrintNotice "Moving u-boot image \"${UBOOT_IMG}\" to: $1"
	cp "${UBOOT_IMG_DIR}/${UBOOT_IMG}" "$1/u-boot.bin"
	cp "${SRC_FIRMWARE_DIR}/u-boot_nand_config.bin" "$1/"
	PrintNotice "Writing u-boot image \"${UBOOT_IMG}\" to: ${SUBPROG_ARG}"
	sudo dd if="${UBOOT_IMG_DIR}/${UBOOT_IMG}" of="${SUBPROG_ARG}" ${BOARD_UBOOT_DD} && sync
}

UBootToRemoteRepo() {
	UploadDataToRemoteRepo "${UBOOT_IMG_DIR}/${UBOOT_IMG}" 'u-boot.bin'
}
