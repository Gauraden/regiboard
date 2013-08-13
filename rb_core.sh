#!/bin/sh

. ./rb_helpers.sh
. ./rb_toolchain.sh
. ./rb_uboot.sh
. ./rb_kernel.sh
. ./rb_rootfs.sh
. ./rb_initramfs.sh
. ./rb_mmc.sh
. ./rb_packet.sh

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
	cat $help_file
}

SelectBoardConfig() {
	ListAllConfigs
	PrintWarn "Please enter name of default configuration:"
	read CORE_SELECT_BOARD
	if [ "${CORE_SELECT_BOARD}" = '' ]; then
		PrintErr 'You should enter the name of config file!'
		SelectBoardConfig
		return 0
	fi
	if ! IsFileExists "${BOARD_DIR}/${CORE_SELECT_BOARD}"; then
		PrintErr "Config file \"${CORE_SELECT_BOARD}\" is not exists!"
		SelectBoardConfig
		return 0
	fi
	DumpCoreSelects
}

GetListOfUnits() {
	PrintNotice "List of available units"
	# TODO
}

if IsFileExists "${CORE_SELECT_CONF}"; then
	Print "Predefined settings:"
	. "${CORE_SELECT_CONF}"
	if [ "${CORE_SELECT_BOARD}" = '' ]; then
		SelectBoardConfig
	fi
	PrintNotice "Board: ${CORE_SELECT_BOARD}"
else
	SelectBoardConfig
fi
