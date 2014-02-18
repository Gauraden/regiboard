#!/bin/sh

_EXPORT_ONLY_FUNCS='y'

. ../../rb_helpers.sh

TARGET_DIR=$1
SRC_SHELL_DIR="../../src/shell"
SRC_ETC_DIR="../../src/etc"

PrintNotice 'Copying linux setup scripts...'
ossetup_dir=${TARGET_DIR}/root/os_setup
if ! IsFileExists "${ossetup_dir}"; then
	mkdir "${ossetup_dir}" || PrintAndDie "Failed to create dir: ${ossetup_dir}"
fi
USR_SBIN_DIR=${TARGET_DIR}/usr/sbin
if ! IsFileExists "${USR_SBIN_DIR}"; then
	PrintAndDie "Failed to find dir: ${USR_SBIN_DIR}"
fi
cp "${SRC_SHELL_DIR}/rb_functions.sh"   "${ossetup_dir}/" || PrintAndDie "Copy failed to: ${ossetup_dir}"
cp "${SRC_SHELL_DIR}/rb_setup_linux.sh" "${ossetup_dir}/" || PrintAndDie "Copy failed to: ${ossetup_dir}"
cp "${SRC_SHELL_DIR}/rb_fuses_imx53.sh" "${ossetup_dir}/" || PrintAndDie "Copy failed to: ${ossetup_dir}"
cp "${SRC_SHELL_DIR}/rb_update_kernel"  "${USR_SBIN_DIR}" || PrintAndDie "Copy failed to: ${USR_SBIN_DIR}"
cp "${SRC_SHELL_DIR}/click"             "${USR_SBIN_DIR}" || PrintAndDie "Copy failed to: ${USR_SBIN_DIR}"
cp "${SRC_SHELL_DIR}/led"               "${USR_SBIN_DIR}" || PrintAndDie "Copy failed to: ${USR_SBIN_DIR}"
cp "${SRC_SHELL_DIR}/udhcpc_renew"      "${USR_SBIN_DIR}" || PrintAndDie "Copy failed to: ${USR_SBIN_DIR}"

PrintNotice 'Copying of system scripts and configs...'
cp "${SRC_SHELL_DIR}/S00gpio_init"     "${TARGET_DIR}/etc/init.d/"
cp "${SRC_SHELL_DIR}/S40network"       "${TARGET_DIR}/etc/init.d/"
cp "${SRC_SHELL_DIR}/S39gui"           "${TARGET_DIR}/etc/init.d/"
cp "${SRC_ETC_DIR}/interfaces"         "${TARGET_DIR}/etc/network/"
cp "${SRC_ETC_DIR}/sshd_config"        "${TARGET_DIR}/etc/"
cp "${SRC_ETC_DIR}/directfbrc"         "${TARGET_DIR}/etc/"

PrintNotice 'Applying of hacks...'
cd "${TARGET_DIR}/usr/share/directfb-1.4.16/" && mv ./cursor.dat ./cursor.dat.bak
# Hacks ------------------------------------------------------------------------
# Samba is not working correct! Removing it from autorun.
# mv "${TARGET_DIR}/etc/init.d/S91smb" "${TARGET_DIR}/root/"
# Was usefull with gcc 4.7
#cd "${TARGETDIR}/lib/" && ln -sf ld-2.9.so ld-linux-armhf.so.3
