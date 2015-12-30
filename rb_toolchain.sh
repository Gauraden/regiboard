#!/bin/sh

TC_AUTOCONF_VER='2.68'

TC_ROOT_DIR="${TOOLCHAIN_DIR}/toolchain.${BOARD_PREFIX}"
TC_BUILD_DIR="${BUILD_DIR}/toolchain.${BOARD_PREFIX}"
TC_CROSSTOOL_DIR="${TOOLCHAIN_DIR}/crosstool.${BOARD_PREFIX}"
TC_CONFIG="${CONF_DIR}/${BOARD_PREFIX}.ct-ng"
TC_PREFIX="${BOARD_ARCH}-${BOARD_PREFIX}-linux-gnueabi"

TC_C='gcc'
TC_CXX='g++'
TC_CPP='cpp'
TC_CTNG='ct-ng'
TC_LDD='ldd'
TC_LD='ld'
TC_AR='ar'
TC_RANLIB='ranlib'

TC_CONFIGURE_FLAGS="CC=${TC_C} CPP=${TC_CPP} CXX=${TC_CXX}"

ConfigurateToolchain() {
	Print "Preparing toolchain for CPU: ${BOARD_CPU} (${BOARD_PREFIX})"
	
	TC_ROOT_DIR="${TOOLCHAIN_DIR}/toolchain.${BOARD_PREFIX}"
	TC_BUILD_DIR="${BUILD_DIR}/toolchain.${BOARD_PREFIX}"
	TC_CROSSTOOL_DIR="${TOOLCHAIN_DIR}/crosstool.${BOARD_PREFIX}"
	TC_CONFIG="${CONF_DIR}/${BOARD_PREFIX}.ct-ng"
	TC_PREFIX="${BOARD_ARCH}-${BOARD_PREFIX}-linux-gnueabi"
	TC_HOST_LOG_ERR="${LOG_DIR}/${BOARD_NAME}.${BOARD_PREFIX}.host_make.err"
	TC_CROS_LOG_ERR="${LOG_DIR}/${BOARD_NAME}.${BOARD_PREFIX}.cross_make.err"
	TC_TARG_LOG_ERR="${LOG_DIR}/${BOARD_NAME}.${BOARD_PREFIX}.target_make.err"

	TC_C="${TC_PREFIX}-${TC_C}"
	TC_CXX="${TC_PREFIX}-${TC_CXX}"
	TC_CPP="${TC_PREFIX}-${TC_CPP}"
	TC_LDD="${TC_PREFIX}-${TC_LDD}"
	TC_LD="${TC_PREFIX}-${TC_LD}"
	TC_AR="${TC_PREFIX}-${TC_AR}"
	TC_RANLIB="${TC_PREFIX}-${TC_RANLIB}"
	
	TC_CONFIGURE_FLAGS="CC=${TC_C} CPP=${TC_CPP} CXX=${TC_CXX}"
	# preparing of environment variable
	#export PATH=${PATH}:${TC_ROOT_DIR}/bin
	#:${TC_CROSSTOOL_DIR}/bin
	# preparing of directories
	if [ "${BOARD_TOOLCHAIN}" != "" ]; then
		BindOrCreateDir "${TC_ROOT_DIR}" "${BOARD_TOOLCHAIN}"	
		return
	fi
	CreateDirIfNotExists "${TC_BUILD_DIR}"
	CreateDirIfNotExists "${TC_ROOT_DIR}"
}

# Build methods for HOST compile
TcHostMakeSources() {
	DieIfNotDefined "$1" "make sources"
	local make_cmd="make -C $1 $2"
	LogIt "${make_cmd}" "${TC_HOST_LOG_ERR}" "Host error: ${make_cmd}"
}

TcHostCleanSources() {
	DieIfNotDefined "$1" "clean sources"
	TcHostMakeSources $1 clean
}

TcHostDistCleanSources() {
	DieIfNotDefined "$1" "distclean sources"
	TcHostMakeSources $1 distclean
}

TcHostRebuildSources() {
	TcHostCleanSources "$1"
	TcHostMakeSources "$1" "$2"
}
# Build methods for CROSS compile
TcCrossMakeSources() {
	DieIfNotDefined "$1" "make sources"
	local make_cmd="make ARCH=${BOARD_ARCH} -C $1 $2"
	LogIt "${make_cmd}" "${TC_CROS_LOG_ERR}" "Cross error: ${make_cmd}"
}

TcCrossCleanSources() {
	DieIfNotDefined "$1" "clean sources"
	TcCrossMakeSources $1 clean
}

TcCrossDistCleanSources() {
	DieIfNotDefined "$1" "distclean sources"
	TcCrossMakeSources $1 distclean
}

TcCrossRebuildSources() {
	TcCrossCleanSources "$1"
	TcCrossMakeSources "$1" "$2"
}
# Build methods for TARGET compile
TcTargetMakeSources() {
	DieIfNotDefined "$1" "make sources"
	local cross_pref="CROSS_COMPILE=${TC_PREFIX}-"
	local make_cmd="make ARCH=${BOARD_ARCH} ${cross_pref} -C $1 $2"
	LogIt "${make_cmd}" "${TC_TARG_LOG_ERR}" "Target error: ${make_cmd}"
}

TcTargetCleanSources() {
	DieIfNotDefined "$1" "clean sources"
	TcTargetMakeSources $1 clean
}

TcTargetDistCleanSources() {
	DieIfNotDefined "$1" "distclean sources"
	TcTargetMakeSources $1 distclean
}

TcTargetRebuildSources() {
	TcTargetCleanSources "$1"
	TcTargetMakeSources "$1" "$2"
}

TcPrepareConfig() {
	PrintNotice "Configuring of toolchain..."
	cp "${TC_CONFIG}" "${TC_BUILD_DIR}/.config"
  local tc_config_path="${TC_BUILD_DIR}/.config"
# Use it only for >=4.7 gcc
#	SetConfigVar      ${tc_config_path} 'CT_TARGET_CFLAGS'           '-mno-unaligned-access'
	
  SetConfigVar      ${tc_config_path} 'CT_PREFIX_DIR'                   "${TC_ROOT_DIR}"
  SetConfigVar      ${tc_config_path} 'CT_KERNEL_LINUX_CUSTOM_LOCATION' "${KERNEL_BUILD_DIR}"
  SetConfigVar      ${tc_config_path} 'CT_LOCAL_TARBALLS_DIR'           "${DOWNLOAD_DIR}"
  SetConfigVar      ${tc_config_path} 'CT_TARGET_VENDOR'                "${BOARD_PREFIX}"
	UndefineConfigVar ${tc_config_path} 'CT_CC_SUPPORT_FORTRAN'
	UndefineConfigVar ${tc_config_path} 'CT_CC_SUPPORT_JAVA'
	UndefineConfigVar ${tc_config_path} 'CT_CC_SUPPORT_ADA'
	UndefineConfigVar ${tc_config_path} 'CT_CC_SUPPORT_OBJC'
	UndefineConfigVar ${tc_config_path} 'CT_CC_SUPPORT_OBJCXX'
	# FORCE_EXTRACT removes symlinks to custom kernel source!	
	UndefineConfigVar ${tc_config_path} 'CT_FORCE_EXTRACT'
}

BuildToolchain() {
	# skip building if external toolchain is defined
	if [ "${BOARD_TOOLCHAIN}" != "" ]; then
		return 0
	fi
	if [ "${SUBPROG_ARG}" = 'once' ] && $TC_C --version | grep -q .; then
	  PrintWarn "Building of toolchain was skipped"
	  return 0
	fi
	local autoconf_tar="autoconf-${TC_AUTOCONF_VER}.tar.gz"
	local crosstool_tar="crosstool-ng-${BOARD_CTNG_VER}.tar.bz2"
	# updating autoconf 
	if ! IsVersion 'autoconf' ${TC_AUTOCONF_VER}; then
		Print "Updating autoconf ..."
		GetFileByUrl "ftp://ftp.gnu.org/gnu/autoconf/${autoconf_tar}" "${DOWNLOAD_DIR}"
		UnpackArchive "${DOWNLOAD_DIR}/${autoconf_tar}" "${BUILD_DIR}"
		PrintNotice "Building autoconf"
		cd "${BUILD_DIR}/autoconf-${TC_AUTOCONF_VER}"
		./configure
		make
		PrintNotice "Installing autoconf"
		sudo make install
	fi
	# checking and fetching crosstool-ng distrib
	local crosstool_build_dir="${BUILD_DIR}/crosstool-ng-${BOARD_CTNG_VER}"
	if ! IsFileExists "${crosstool_build_dir}"; then
		GetFileByUrl "http://crosstool-ng.org/download/crosstool-ng/${crosstool_tar}" "${DOWNLOAD_DIR}"
		UnpackArchive "${DOWNLOAD_DIR}/${crosstool_tar}" "${BUILD_DIR}"
	fi
	# reinstalling crosstool-ng
	rm -r -f "${TC_CROSSTOOL_DIR}"
	PrintNotice "Preparing crosstool-ng..."
	cd "${crosstool_build_dir}"
	./configure --prefix="${TC_CROSSTOOL_DIR}"
	make
	make install
	# remove existing toolchain
	local kernel_symlink=${TC_BUILD_DIR}/.build/src/linux-custom/linux-${BOARD_KERNEL_VER}
	rm -r -f "${TC_ROOT_DIR}/*"
	IsFileExists $kernel_symlink && rm $kernel_symlink
	# configuring of toolchain
	TcPrepareConfig
	local ctng_bin="${TC_CROSSTOOL_DIR}/bin/${TC_CTNG}"
	if [ "${SUBPROG_ARG}" = 'config' ]; then
		PrintNotice "Editing CrossTool-NG config..."
		${ctng_bin} -C "${TC_BUILD_DIR}" menuconfig
		return 0
	fi
	PrintNotice "Building toolchain..."
	${ctng_bin} -C "${TC_BUILD_DIR}" build
	# some hacks, was usefull with gcc 4.7
#	chmod +755 "${TC_ROOT_DIR}/${TC_PREFIX}/lib"
#	cd "${TC_ROOT_DIR}/${TC_PREFIX}/lib" && ln -s ld-2.9.so ld-linux-armhf.so.3
	NotifyUser "Building of toolchain for \"${BOARD_ARCH}.${BOARD_PREFIX}\" was finished!"
}

