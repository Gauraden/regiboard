#!/bin/sh

ConfigurateKernel() {
	DieIfNotDefined ${BOARD_KERNEL_VER} "kernel version"
	DieIfNotDefined ${BOARD_KERNEL_CNF} "kernel configuration"
  if ! test ${BOARD_KERNEL_VER}; then
	  BOARD_KERNEL_VER="${BOARD_KERNEL_BRANCH}"
	fi
	KERNEL_IMG="uImage.${BOARD_NAME}.${BOARD_CPU}.bin"
	KERNEL_DIR="linux-${BOARD_KERNEL_VER}"
	KERNEL_BUILD_DIR="${BUILD_DIR}/${KERNEL_DIR}"
	KERNEL_CONFIG="${CONF_DIR}/${BOARD_KERNEL_CNF}"
	KERNEL_MODULES_DIR="${KERNEL_IMG_DIR}/modules.${BOARD_NAME}.${BOARD_CPU}"
	CreateDirIfNotExists "${KERNEL_MODULES_DIR}"
	# We need it here because of toolchain dependency
	# пробуем спулить проект
  if IsFileExists "${KERNEL_BUILD_DIR}/.git"; then
    pushd ${KERNEL_BUILD_DIR} && git pull origin && popd
  fi
	if ! IsFileExists ${KERNEL_BUILD_DIR}; then
	  git clone ${BOARD_KERNEL_GIT} ${KERNEL_BUILD_DIR} && pushd ${KERNEL_BUILD_DIR} && git checkout ${BOARD_KERNEL_BRANCH} && popd
    # если не удалось скачать исходники из репозитория
		if ! IsFileExists ${KERNEL_BUILD_DIR}; then
  		UnpackArchive "${DOWNLOAD_DIR}/${KERNEL_DIR}.tar.bz2" "${BUILD_DIR}"
	  	ApplyAllPatchesFor "${KERNEL_DIR}" "${KERNEL_BUILD_DIR}"
	  fi
	fi
}

SwapKernelConfig() {
	SwapConfig "${KERNEL_BUILD_DIR}" "${KERNEL_CONFIG}"
}

CancelKernel() {
	CancelSwapConfig "${KERNEL_BUILD_DIR}"
}

BuildKernel() {
	SwapKernelConfig
	# setup init RAM FS image
	local config="${KERNEL_BUILD_DIR}/.config"
	SetConfigVar "${config}" 'CONFIG_INITRAMFS_SOURCE' "${IRAMFS_BUILD_DIR}"
	SetConfigVar "${config}" 'CONFIG_CROSS_COMPILE'    "${TC_PREFIX}"
	if [ "${SUBPROG_ARG}" = 'config' ]; then
		PrintNotice "Editing kernel config: ${BOARD_KERNEL_CNF}"
		TcCrossMakeSources "${KERNEL_BUILD_DIR}" menuconfig
		SwapKernelConfig
		return 0
	fi
	if [ "${SUBPROG_ARG}" = 'mkpatch' ]; then
		CreatePatch "${KERNEL_BUILD_DIR}"
		return 0
	fi
	if [ "${SUBPROG_ARG}" = 'clean' ]; then
		PrintNotice "Clearing kernel sources..."
		TcTargetCleanSources "${KERNEL_BUILD_DIR}"
	fi
	PrintNotice "Building kernel: ${BOARD_KERNEL_CNF}"
	TcTargetMakeSources "${KERNEL_BUILD_DIR}" uImage 'CROSS_COMPILE'
	PrintNotice "Building modules..."
	TcTargetMakeSources "${KERNEL_BUILD_DIR}" modules 'CROSS_COMPILE'
	PrintNotice "Installing modules: $KERNEL_MODULES_DIR"
	TcTargetMakeSources "${KERNEL_BUILD_DIR}" "INSTALL_MOD_PATH=\"${KERNEL_MODULES_DIR}\" modules_install"
	PrintNotice "Copying image to output directory..."
	mv "${KERNEL_BUILD_DIR}/arch/${BOARD_ARCH}/boot/uImage" "${KERNEL_IMG_DIR}/${KERNEL_IMG}"
	SwapKernelConfig
	NotifyUser "Building of linux kernel \"${BOARD_KERNEL_CNF}\" was finished!"
}

KernelToMMC() {
	DieIfNotDefined $1 "boot partition is not defined"
	if ! IsFileExists "$1"; then
		PrintAndDie "Invalid path: $1"
	fi
	if [ ! -d "$1" ]; then
		PrintAndDie "It's not directory: $1"
	fi
	PrintNotice "Moving kernel image \"${KERNEL_IMG}\" to: $1"
	cp "${KERNEL_IMG_DIR}/${KERNEL_IMG}" "$1/uImage"
# TODO: kernel modules setup
#	PrintNotice "Moving kernel modules to: $1"
#	rm -r -f "$1/modules"
#	cp -r "${KERNEL_MODULES_DIR}" "$1/modules"
}

KernelToRemoteRepo() {
	UploadDataToRemoteRepo "${KERNEL_IMG_DIR}/${KERNEL_IMG}" 'uImage'
}
