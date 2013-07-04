#!/bin/sh

_EXPORT_ONLY_FUNCS='y'

. ../../rb_helpers.sh

TARGET_DIR=$1
SRC_SHELL_DIR="../../src/shell"
SRC_ETC_DIR="../../src/etc"

PrintNotice 'Copying linux setup scripts...'
ossetup_dir=${TARGET_DIR}/root/os_setup
if ! IsFileExists "${ossetup_dir}"; then
	mkdir "${ossetup_dir}" || PrintAndDie "Failed to create dir: ${setup_dir}"
fi
cp "${SRC_SHELL_DIR}/rb_functions.sh"   "${ossetup_dir}/" || PrintAndDie "Copy failed to: ${ossetup_dir}"
cp "${SRC_SHELL_DIR}/rb_setup_linux.sh" "${ossetup_dir}/" || PrintAndDie "Copy failed to: ${ossetup_dir}"

PrintNotice 'Copying of system scripts and configs...'
cp "${SRC_SHELL_DIR}/S41eth0_restart"  "${TARGET_DIR}/etc/init.d/"
cp "${SRC_SHELL_DIR}/S60gui"           "${TARGET_DIR}/etc/init.d/"
cp "${SRC_ETC_DIR}/interfaces"         "${TARGET_DIR}/etc/network/"
cp "${SRC_ETC_DIR}/sshd_config"        "${TARGET_DIR}/etc/"
cp "${SRC_ETC_DIR}/directfbrc"        "${TARGET_DIR}/etc/"
# Hacks ------------------------------------------------------------------------
# Was usefull with gcc 4.7
#cd "${TARGETDIR}/lib/" && ln -sf ld-2.9.so ld-linux-armhf.so.3
