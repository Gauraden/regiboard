#!/bin/bash

_EXPORT_ONLY_FUNCS='y'

. ../../rb_helpers.sh

TARGET_DIR=$1
TARGET_TTY_FONT="${TARGET_DIR}/usr/share/fonts/tty"

SRC_SHELL_DIR="../../src/shell"
SRC_ETC_DIR="../../src/etc"
SRC_FONTS_DIR="../../src/tty_font"

#PrintNotice 'Copying "sysroot" from toolchain...'
#cp -r $TARGET_DIR/../../toolchain/sysroot/* ${TARGET_DIR}/

PrintNotice 'Copying fonts...'
mkdir ${TARGET_TTY_FONT}
cp ${SRC_FONTS_DIR}/* ${TARGET_TTY_FONT}/

echo "cp ${SRC_FONTS_DIR}/* ${TARGET_TTY_FONT}/"

PrintNotice 'Copying linux setup scripts...'
OS_SETUP_DIR=${TARGET_DIR}/root/os_setup
USR_SBIN_DIR=${TARGET_DIR}/usr/sbin

CreateDirIfNotExists "${OS_SETUP_DIR}"
IsFileExists "${USR_SBIN_DIR}" || PrintAndDie "Failed to find dir: ${USR_SBIN_DIR}"

cp "${SRC_SHELL_DIR}/click"        "${USR_SBIN_DIR}" || PrintAndDie "Copy failed to: ${USR_SBIN_DIR}"
cp "${SRC_SHELL_DIR}/led"          "${USR_SBIN_DIR}" || PrintAndDie "Copy failed to: ${USR_SBIN_DIR}"
cp "${SRC_SHELL_DIR}/udhcpc_renew" "${USR_SBIN_DIR}" || PrintAndDie "Copy failed to: ${USR_SBIN_DIR}"

PrintNotice 'Copying of system scripts and configs...'
cp "${SRC_SHELL_DIR}/S00gpio_init"     "${TARGET_DIR}/etc/init.d/"
cp "${SRC_SHELL_DIR}/S11ubi"           "${TARGET_DIR}/etc/init.d/"
cp "${SRC_SHELL_DIR}/S40network"       "${TARGET_DIR}/etc/init.d/"
cp "${SRC_SHELL_DIR}/S51sysinfo"       "${TARGET_DIR}/etc/init.d/"
cp "${SRC_SHELL_DIR}/sysinfo.sh"       "${TARGET_DIR}/usr/lib/"
cp "${SRC_ETC_DIR}/interfaces"         "${TARGET_DIR}/etc/network/"
cp "${SRC_ETC_DIR}/sshd_config"        "${TARGET_DIR}/etc/"
cp "${SRC_ETC_DIR}/directfbrc"         "${TARGET_DIR}/etc/"
cp "${SRC_ETC_DIR}/ipkg.conf"          "${TARGET_DIR}/etc/"

PrintNotice 'Generating of MAC...'
HW_ADDR=$(printf '00:60:2F:%02X:%02X:%02X' $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256)))
Print "Addr: ${HW_ADDR}"
echo "$HW_ADDR" > ${TARGET_DIR}/etc/hwaddr

PrintNotice 'Applying of hacks...'
cd "${TARGET_DIR}/usr/share/directfb-1.4.16/" && mv ./cursor.dat ./cursor.dat.bak || echo
# Hacks ------------------------------------------------------------------------
# Samba is not working correct! Removing it from autorun.
rm -f "${TARGET_DIR}/etc/init.d/S91smb"
# Was usefull with gcc 4.7
#cd "${TARGETDIR}/lib/" && ln -sf ld-2.9.so ld-linux-armhf.so.3
