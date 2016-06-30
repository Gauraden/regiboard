#!/bin/sh

IRAMFS_BUILD_DIR="${BUILD_DIR}/initramfs.${TARGET_NAME_PREFIX}"

ConfigurateInitRAMFS() {
	Print "Preparing for building image of RAM FS..."
	IRAMFS_BUILD_DIR="${BUILD_DIR}/initramfs.${TARGET_NAME_PREFIX}"
	CreateDirIfNotExists "${IRAMFS_BUILD_DIR}"
}

BuildBusyBoxOld() {
	local busybox_dir="${RFS_BUILD_DIR}/output/build/busybox-${BOARD_BUSYBOX_VER}"
	if ! IsFileExists "${busybox_dir}"; then
		PrintWarn "Busybox sources was not found: ${busybox_dir}"
		PrintNotice "Let's try autodetection..."
		busybox_dir=$(ls -d "${RFS_BUILD_DIR}/output/build/busybox*")
	fi
	cp "${CONF_DIR}/busybox" "${busybox_dir}/.config"
	local config="${busybox_dir}/.config"	
	SetConfigVar ${config} 'CONFIG_CROSS_COMPILER_PREFIX' "${TC_PREFIX}-"
	SetConfigVar ${config} 'CONFIG_SYSROOT'               "${RFS_ROOT_DIR}/staging"
#	SetConfigVar ${config} 'CONFIG_EXTRA_CFLAGS'          "-mno-unaligned-access"
	if [ "${SUBPROG_ARG}" = 'config' ]; then
		PrintNotice "Editing BusyBox config..."
#		TcCrossMakeSources "${busybox_dir}" menuconfig
		TcTargetMakeSources "${busybox_dir}" menuconfig
		cp "${busybox_dir}/.config" "${CONF_DIR}/busybox"
		return 0
	fi
	TcTargetCleanSources "${busybox_dir}"
	TcTargetMakeSources  "${busybox_dir}"
	mv "${busybox_dir}/busybox" "${IRAMFS_BUILD_DIR}/bin/"
}

BuildBusyBox() {
  ./rb_build.sh packets busybox
 	PrintNotice "Creating busybox symlinks..."
 	local busybox="busybox"
 	local symlink=""
 	cd "${IRAMFS_BUILD_DIR}/bin/"
 	
 	MkAlias() {
   	ln -sf $busybox "${symlink}${1}"
 	}
 	
  MkAlias 'sync'
  MkAlias 'bash'
  MkAlias 'cat'
  MkAlias 'dd'
  MkAlias 'echo'
  MkAlias 'grep'
  MkAlias 'hush'
  MkAlias 'ifconfig'
  MkAlias 'ls'
  MkAlias 'mkdir'
  MkAlias 'mknod'
  MkAlias 'mount'
  MkAlias 'umount'
  MkAlias 'sed'
  MkAlias 'sh'
  MkAlias 'sleep'
  MkAlias 'switch_root'
  MkAlias 'tar'
  MkAlias 'gunzip'
  MkAlias 'test'
  MkAlias 'tftp'
  MkAlias 'ubiattach'
  MkAlias 'ubidetach'
  MkAlias 'ubimkvol'
  MkAlias 'udhcpc'
  MkAlias 'rmmod'
  MkAlias 'modprobe'
}

PrepareInitRAMFS() {
  local use_sudo='sudo'
	PrintNotice "Clearing init RAM FS structure..."
	${use_sudo} rm -r "${IRAMFS_BUILD_DIR}/*" 2> ${_DEV_NULL}
	# TODO: i think it`ll be better to use "mkinitramfs" script
	# creating of directories structure
	PrintNotice "Creating directories structure..."
	CreateDirIfNotExists "${IRAMFS_BUILD_DIR}/bin"
	CreateDirIfNotExists "${IRAMFS_BUILD_DIR}/dev"
	CreateDirIfNotExists "${IRAMFS_BUILD_DIR}/etc"
	CreateDirIfNotExists "${IRAMFS_BUILD_DIR}/lib"
#	CreateDirIfNotExists "${IRAMFS_BUILD_DIR}/bin"
	CreateDirIfNotExists "${IRAMFS_BUILD_DIR}/mnt"
	CreateDirIfNotExists "${IRAMFS_BUILD_DIR}/proc"
#	CreateDirIfNotExists "${IRAMFS_BUILD_DIR}/bin"
	CreateDirIfNotExists "${IRAMFS_BUILD_DIR}/root"
#	CreateDirIfNotExists "${IRAMFS_BUILD_DIR}/bin"
	CreateDirIfNotExists "${IRAMFS_BUILD_DIR}/sbin"
#	CreateDirIfNotExists "${IRAMFS_BUILD_DIR}/bin"
	CreateDirIfNotExists "${IRAMFS_BUILD_DIR}/sys"
  CreateDirIfNotExists "${IRAMFS_BUILD_DIR}/tmp"
	# creating dev nodes
	PrintNotice "Creating devices..."
	${use_sudo} mknod "${IRAMFS_BUILD_DIR}/dev/console"   c 5   1
	${use_sudo} mknod "${IRAMFS_BUILD_DIR}/dev/fb0"       c 29  0
	${use_sudo} mknod "${IRAMFS_BUILD_DIR}/dev/null"      c 1   3
	${use_sudo} mknod "${IRAMFS_BUILD_DIR}/dev/ttymxc0"   c 207 16
#	${use_sudo} mknod "${IRAMFS_BUILD_DIR}/dev/ubi0"      c 247 0
	${use_sudo} mknod "${IRAMFS_BUILD_DIR}/dev/ubi_ctrl"  c 10  61
	${use_sudo} mknod "${IRAMFS_BUILD_DIR}/dev/mmcblk0p1" b 179 1
	${use_sudo} mknod "${IRAMFS_BUILD_DIR}/dev/mmcblk0p2" b 179 2
}

BuildInitRAMFS() {
	local init_file="${SRC_SHELL_DIR}/init"
	if ! IsFileExists "${init_file}"; then
		PrintAndDie "Init file is not exists: ${init_file}"
	fi
	if [ "${SUBPROG_ARG}" = 'clean' ] || \
	     ! IsFileExists "${IRAMFS_BUILD_DIR}/dev"; then
		PrepareInitRAMFS
	fi
	# сборка зависимостей
  PrintNotice "Building static mtd-utils..."
	./rb_build.sh packets mtd
	PrintNotice "Building regisplash..."
	./rb_build.sh packets regisplash
	PrintNotice "Building static busybox..."
	BuildBusyBox
	# prepare utils and scripts
	PrintNotice "Installing utils and scripts..."
	cp "${init_file}" "${IRAMFS_BUILD_DIR}/sbin/"
	cd ${IRAMFS_BUILD_DIR}
	ln -sf sbin/init init
	NotifyUser "Building of init RAM FS for \"${TARGET_NAME_PREFIX}\" was finished!"
}
