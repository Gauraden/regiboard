#!/bin/sh

ConfigurateUBoot() {
	DieIfNotDefined ${BOARD_UBOOT_VER} "u-boot version"
	DieIfNotDefined ${BOARD_UBOOT_CNF} "u-boot configuration"
	if ! test ${BOARD_UBOOT_VER}; then
	  BOARD_UBOOT_VER="${BOARD_UBOOT_BRANCH}"
	fi
	UBOOT_IMG="u-boot.${TARGET_NAME_CPU}.bin"
	UBOOT_DIR="u-boot-${BOARD_UBOOT_VER}"
	UBOOT_BUILD_DIR="${BUILD_DIR}/${UBOOT_DIR}"
}

ConvertBinToImx() {
	PrintNotice "Creating u-boot.imx image"
	local output_dir=$1
	DieIfNotDefined ${output_dir} "output directory for *.imx image"
	IsFileExists ${output_dir} || (PrintErr "directory not found: ${output_dir}" && return)
	local mkimg="${BIN_DIR}/${TARGET_NAME_CPU}/mkimage"
	local nand_cfg="${CONF_DIR}/mkimage/imximage_nand.cfg"
	local sd_cfg="${CONF_DIR}/mkimage/imximage_sd.cfg"
	local bin_img="${UBOOT_IMG_DIR}/${UBOOT_IMG}"
	local imx_img="${output_dir}/u-boot.${TARGET_NAME_CPU}"
  $mkimg -n $nand_cfg -T imximage -e 0x77800000 -d $bin_img ${imx_img}.nand.imx
  cksum ${imx_img}.nand.imx
	$mkimg -n $sd_cfg   -T imximage -e 0x77800000 -d $bin_img ${imx_img}.sd.imx
	cksum ${imx_img}.sd.imx
	ln -s ${imx_img}.nand.imx ${imx_img}.imx 2> ${_DEV_NULL}
}

BuildUBoot() {
  # пробуем спулить проект
  if IsFileExists "${UBOOT_BUILD_DIR}/.git"; then
    pushd ${UBOOT_BUILD_DIR} && git pull origin && popd
  fi
  # если не удалось спулить, тогда распаковываем архив
	if ! IsFileExists ${UBOOT_BUILD_DIR}; then
		# Downloading
		git clone ${BOARD_UBOOT_GIT} ${UBOOT_BUILD_DIR} && pushd ${UBOOT_BUILD_DIR} && git checkout ${BOARD_UBOOT_BRANCH} && popd
		# если не удалось скачать исходники из репозитория
		if ! IsFileExists ${UBOOT_BUILD_DIR}; then
	  	# Unpacking
	  	UnpackArchive "${DOWNLOAD_DIR}/${UBOOT_DIR}.tar.bz2" "${BUILD_DIR}"
  		ApplyAllPatchesFor "${UBOOT_DIR}" "${UBOOT_BUILD_DIR}"
		fi
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
	TcTargetMakeSources ${UBOOT_BUILD_DIR} "${BOARD_UBOOT_CNF}_config" 'CROSS_COMPILE'
	PrintNotice "Building u-boot.bin"
	local pswd_hash=$(${UTIL_UBOOTPSWD} --pswd="$UBOOT_PSWD" | sed -r 's/(.*)md5(.+): 0x(.+)/\3/')
	export EXTERN_DEFS="-DCONFIG_PSWD=\\\"$pswd_hash\\\""
	TcTargetMakeSources ${UBOOT_BUILD_DIR} 'all' 'CROSS_COMPILE'
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
