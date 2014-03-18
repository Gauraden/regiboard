#!/bin/sh

. ./rb_helpers.sh
. ./rb_toolchain.sh
. ./rb_uboot.sh
. ./rb_kernel.sh
. ./rb_rootfs.sh
. ./rb_initramfs.sh
. ./rb_mmc.sh
. ./rb_packet.sh
. ./rb_firmware.sh

CORE_SELECT_CONF="${TMP_DIR}/core_select.conf"
CORE_SELECT_BOARD=''

DumpCoreSelects() {
	echo "CORE_SELECT_BOARD=\"${CORE_SELECT_BOARD}\"" > "${CORE_SELECT_CONF}"
}

PrintHelp() {
	local help_file="${WORK_DIR}/readme_${_SYS_LOCALE}.txt"
	if ! IsFileExists $help_file; then
		help_file="${WORK_DIR}/readme_en.txt"
	fi
	PrintWithColors $help_file
}

SelectBoardConfig() {
	ListAllConfigs
	PrintWarn "Please enter name of default configuration:"
	read CORE_SELECT_BOARD
  IsDefined ${CORE_SELECT_BOARD} || \
	(	PrintErr 'You should enter the name of config file!' && \
		SelectBoardConfig && \
		return 0)
  IsFileExists "${BOARD_DIR}/${CORE_SELECT_BOARD}" || \
  (	PrintErr "Config file \"${CORE_SELECT_BOARD}\" is not exists!" && \
		SelectBoardConfig && \
		return 0)
	DumpCoreSelects
}

GetListOfUnits() {
	PrintNotice "List of available units"
	# TODO
}

if IsFileExists "${CORE_SELECT_CONF}"; then
	Print "Predefined settings:"
	. "${CORE_SELECT_CONF}"
	IsDefined ${CORE_SELECT_BOARD} ||	SelectBoardConfig
	PrintNotice "Board: ${CORE_SELECT_BOARD}"
else
	SelectBoardConfig
fi
