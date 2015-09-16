#!/bin/sh

RFS_BUILDROOT_CONFIG="${CONF_DIR}/${BOARD_NAME}.buildroot"
RFS_BUILDROOT_NAME="buildroot.${BOARD_NAME}.${BOARD_PREFIX}"
RFS_ROOT_DIR="${BUILDROOT_DIR}/${RFS_BUILDROOT_NAME}"
RFS_BUILD_DIR="${BUILD_DIR}/${RFS_BUILDROOT_NAME}"
RFS_HOST_DIR="${RFS_ROOT_DIR}/host"
RFS_IMAGE="rootfs.${BOARD_NAME}.${BOARD_PREFIX}.tar"

ConfigurateRootFS() {
	Print "Preparing BuildRoot for: ${BOARD_NAME}"
	RFS_BUILDROOT_CONFIG="${CONF_DIR}/${BOARD_NAME}.buildroot"
	RFS_BUILDROOT_NAME="buildroot.${BOARD_NAME}.${BOARD_PREFIX}"
	RFS_ROOT_DIR="${BUILDROOT_DIR}/${RFS_BUILDROOT_NAME}"
	RFS_BUILD_DIR="${BUILD_DIR}/${RFS_BUILDROOT_NAME}"
	RFS_HOST_DIR="${RFS_ROOT_DIR}/host"
	RFS_IMAGE="rootfs.${BOARD_NAME}.${BOARD_PREFIX}.tar"
	# preparing of environment variable
	export PATH="${PATH}:${RFS_HOST_DIR}/usr/bin"
	if [ "${SUBPROG_TYPE}" = 'packets' ]; then
		export CPPFLAGS="$CPPFLAGS --sysroot=${RFS_ROOT_DIR}/staging"
		export CFLAGS="$CFLAGS -I${RFS_ROOT_DIR}/staging/usr/include -L${RFS_ROOT_DIR}/target/usr/lib"
		export CXXFLAGS="$CXXFLAGS -I${RFS_ROOT_DIR}/staging/usr/include -L${RFS_ROOT_DIR}/target/usr/lib"
		export LDFLAGS="$LDFLAGS -Wl,-rpath-link,-L${RFS_ROOT_DIR}/target/usr/lib"
	fi
	# check configuration of board, may be user want to use external BuildRoot
	if [ "$BOARD_BUILDROOT" != "" ]; then
		if IsFileExists "${RFS_ROOT_DIR}"; then
			PrintWarn "BuildRoot is already exists, ignoring BOARD_BUILDROOT option!"
			return
		fi
		BindOrCreateDir "${RFS_ROOT_DIR}" "${BOARD_BUILDROOT}/output"
		return
	fi
	# preparing for internal build
#	CreateDirIfNotExists "${RFS_BUILD_DIR}"
}

SwapBuildRootConfig() {
	SwapConfig "${RFS_BUILD_DIR}" "${RFS_BUILDROOT_CONFIG}"
}

CancelRootFS() {
	CancelSwapConfig "${RFS_BUILD_DIR}"
}

RFSPrepareConfig() {
	PrintNotice "Configuring of BuildRoot..."
	local br_config_path="${RFS_BUILD_DIR}/.config"
	SetConfigVar ${br_config_path} 'BR2_TOOLCHAIN_EXTERNAL_PATH'          "${TC_ROOT_DIR}"
	SetConfigVar ${br_config_path} 'BR2_TOOLCHAIN_EXTERNAL_CUSTOM_PREFIX' "${TC_PREFIX}"
	SetConfigVar ${br_config_path} 'BR2_TOOLCHAIN_EXTERNAL_PREFIX'        "${TC_PREFIX}"
	SetConfigVar ${br_config_path} 'BR2_TARGET_GENERIC_HOSTNAME'          "${BOARD_NAME}"
	SetConfigVar ${br_config_path} 'BR2_TARGET_GENERIC_ISSUE'             "Welcome to RegiBoard [${BOARD_CONFIG}]"
	SetConfigVar ${br_config_path} 'BR2_ROOTFS_POST_BUILD_SCRIPT'         "${WORK_DIR}/rb_patch_rootfs_image.sh"
	SetConfigVar ${br_config_path} 'BR2_PACKAGE_BUSYBOX_CONFIG'           "${CONF_DIR}/busybox-1.20.x.config"
	# Enable UDev
	SetConfigVar ${br_config_path} 'BR2_ROOTFS_DEVICE_CREATION_DYNAMIC_UDEV' 'y'
	UndefineConfigVar ${br_config_path} 'BR2_ROOTFS_DEVICE_CREATION_STATIC'
	UndefineConfigVar ${br_config_path} 'BR2_ROOTFS_DEVICE_CREATION_DYNAMIC_DEVTMPFS'
	UndefineConfigVar ${br_config_path} 'BR2_ROOTFS_DEVICE_CREATION_DYNAMIC_MDEV'
#	UndefineConfigVar ${br_config_path} 'BR2_PREFER_SOFT_FLOAT'
}

BuildRootFS() {
	# downloading of fresh BuildRoot sources
#	AttachGitProject 'http://git.buildroot.net/git/buildroot.git' "${TMP_DIR}"
#	rm -r "${RFS_BUILD_DIR}/package"
#	cp -r $(ls --ignore '.git' "${TMP_DIR}/buildroot") "${RFS_BUILD_DIR}/"
	if ! IsFileExists ${RFS_BUILD_DIR}; then
		# Downloading
		# TODO
		# Unpacking
		UnpackArchive "${DOWNLOAD_DIR}/buildroot-${BOARD_BUILDROOT_VER}.tar.gz" "${BUILD_DIR}"
		mv "${BUILD_DIR}/buildroot-${BOARD_BUILDROOT_VER}" "${BUILD_DIR}/${RFS_BUILDROOT_NAME}"
		ApplyAllPatchesFor "${RFS_BUILDROOT_NAME}" "${RFS_BUILD_DIR}"
	fi
	SwapBuildRootConfig
	if [ "$BOARD_BUILDROOT" = "" ]; then
		RFSPrepareConfig
	fi
	if [ "${SUBPROG_ARG}" = 'config' ]; then
		PrintNotice "Editing BuildRoot config..."
		TcCrossMakeSources "${RFS_BUILD_DIR}" menuconfig
		SwapBuildRootConfig
		return 0
	fi
	if [ "${SUBPROG_ARG}" = 'mkpatch' ]; then
		CreatePatch "${RFS_BUILD_DIR}"
		return 0
	fi
	if [ "${SUBPROG_ARG}" = 'clean' ]; then
		PrintNotice "Clearing BuildRoot sources..."
		TcTargetCleanSources "${RFS_BUILD_DIR}"
	fi
	# setup directory for downloads
	BindDirIfRequired "${DOWNLOAD_DIR}" "${RFS_BUILD_DIR}/dl"
	# setup directory for kernel modules
	mkdir -p "${RFS_BUILD_DIR}/output/target/lib"
	ln -fs "${KERNEL_MODULES_DIR}" "${RFS_BUILD_DIR}/output/target/lib/modules" || 
	  PrintAndDie 'Directory with kernel modules was not found!'
	# цепляем sysroot toolchain`a к buildroot
	cp -r ${TC_ROOT_DIR}/${TC_PREFIX}/sysroot ${RFS_BUILD_DIR}/toolchain/
	chmod -R a+w ${RFS_BUILD_DIR}/toolchain/sysroot
  rm -r ${RFS_BUILD_DIR}/toolchain/sysroot/usr/include
	# building root FS
	PrintNotice "Building root file system..."
	TcTargetMakeSources  "${RFS_BUILD_DIR}"
	SwapBuildRootConfig
	PrintNotice "Copying root file system image to output directory..."
	mv "${RFS_ROOT_DIR}/images/rootfs.tar" \
	   "${ROOTFS_IMG_DIR}/${RFS_IMAGE}"
	BindDirIfRequired "${RFS_BUILD_DIR}/output" "${RFS_ROOT_DIR}"
	NotifyUser "Building of root FS for \"${BOARD_NAME}.${BOARD_PREFIX}\" was finished!"
}

RootFSValidation() {
	Print "Validating root FS"
	PrintNotice "Checking BusyBox..."
	# Let's check linkage of busybox with systems libraries (through linuxrc)
	${TC_LDD} --root "$1" "$1/linuxrc"
	# Print out busybox header (through linuxrc)
	readelf -h "$1/linuxrc"
}

RootFSToMMC() {
	DieIfNotDefined $1 "rootfs partition is not defined"
	if ! IsFileExists "$1"; then
		PrintAndDie "Invalid path: $1"
	fi
	if [ ! -d "$1" ]; then
		PrintAndDie "It's not directory: $1"
	fi
	if [ "$1" = "/" ]; then
		PrintAndDie "You decided to clear root of your OS?: $1"
	fi
	PrintNotice "Clearing root file system partition: $1"
	sudo rm -r "$1/*" 2> ${_DEV_NULL}
	UseSudo && UnpackArchive "${ROOTFS_IMG_DIR}/${RFS_IMAGE}" "$1"
	PrintNotice 'Patching system files...'
	sudo sh -c "echo '/dev/mmcblk0p1 /boot vfat defaults 0 0' >> '$1/etc/fstab'"
	sudo mkdir "$1/boot"
	local setup_dir="$1/root/os_setup"
	PrintNotice 'Copying linux setup files...'
	if IsFileExists "${setup_dir}"; then
		sudo cp "${ROOTFS_IMG_DIR}/${RFS_IMAGE}" "${setup_dir}/rootfs.tar"
	else
		PrintWarn "Failed to copy files, directory is not exists: ${setup_dir}"
	fi
	RootFSValidation "$1"
}

RootFSToRemoteRepo() {
	UploadDataToRemoteRepo "${ROOTFS_IMG_DIR}/${RFS_IMAGE}" 'rootfs.tar'
}
