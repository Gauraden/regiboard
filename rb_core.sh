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
REMOTE_REPO_LIST="${TMP_DIR}/.listing"

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

CheckSourcesOfPackets() {
  local list_old="$REMOTE_REPO_LIST.old"
  local dl_empty=true
  Print "Detecting sources..."
  find $DOWNLOAD_DIR -mindepth 1 -print -quit | grep -q . && dl_empty=false
  mv -f $REMOTE_REPO_LIST $list_old 2> $_DEV_NULL
  if [ -f $list_old ] && [ $dl_empty == false ]; then
    GetFtpDirList $REPO_PACKETS
    local is_diff=$(diff -q -w -B -Z -E "$REMOTE_REPO_LIST" "$list_old" 2> $_DEV_NULL)
    test "$is_diff" == '' && return
  fi
  GetFtpDir $REPO_PACKETS $DOWNLOAD_DIR
}

if IsFileExists "${CORE_SELECT_CONF}"; then
	Print "Predefined settings:"
	. "${CORE_SELECT_CONF}"
	IsDefined ${CORE_SELECT_BOARD} ||	SelectBoardConfig
	PrintNotice "Board: ${CORE_SELECT_BOARD}"
else
	SelectBoardConfig
fi
