#!/bin/sh

FinaliseMMC() {
	PrintNotice "Synchronisation..."
	sync
	if DoYouWantContinue "Do you want unmount MMC partitions?"; then
		sudo umount "${MMC_BOOT_PART}"
		sudo umount "${MMC_ROOTFS_PART}"
	fi
	exit 0
}

BuildMMC() {
	# let's remember last used device
	local func_cache="${TMP_DIR}/build_mmc_cache"
	if [ "${SUBPROG_ARG}" != "" ]; then
		echo "${SUBPROG_ARG}" > "${func_cache}"
	else
		SUBPROG_ARG=$(cat "${func_cache}")
		PrintWarn "Device \"${SUBPROG_ARG}\" will be used!"
	fi
	# Let us check the name of device, and warn user if that name seems strange
	if [ "${SUBPROG_ARG}" = '/dev/sda' ] && \
	   ! DoYouWantContinue "Do you realy want to write image to: ${SUBPROG_ARG} ?"; then
		exit 0
	fi
	MMC_BOOT_PART=$(GetMountPointOfDevice "${SUBPROG_ARG}1" 'vfat')
	MMC_ROOTFS_PART=$(GetMountPointOfDevice "${SUBPROG_ARG}2" 'ext2')
	DieIfNotDefined "${MMC_BOOT_PART}"   "path for boot partition not found"
	DieIfNotDefined "${MMC_ROOTFS_PART}" "path for root FS partition not found"
	if ! IsFileExists "${MMC_BOOT_PART}"; then
		PrintAndDie "Boot partition was not found on MMC: ${SUBPROG_ARG}1"
	fi
	if ! IsFileExists "${MMC_ROOTFS_PART}"; then
		PrintAndDie "Root FS partition was not found on MMC: ${SUBPROG_ARG}2"
	fi
	# Writing u-boot
	UBootToMMC "${MMC_BOOT_PART}"
	# Writing kernel
	KernelToMMC "${MMC_BOOT_PART}"
	# Writing rootfs
	if ! DoYouWantContinue "Do you want update root file system: ${MMC_ROOTFS_PART} ?"; then
		FinaliseMMC
	fi
	RootFSToMMC "${MMC_ROOTFS_PART}"
	FinaliseMMC
}
