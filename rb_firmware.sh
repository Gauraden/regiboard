#!/bin/sh

FirmwareList() {
	Print "Available firmware:"
#	for packet in $(ls "${PAK_CONF_DIR}" | sed -r 's:(.+)\.conf:\1:'); do
#		PrintNotice "$packet"
#	done
}

FirmwareCreate() {
  local firmw_conf=$1
  DieIfNotDefined $1 "firmware config name"
}

BuildFirmware() {
  IsFileExists "${CONF_FIRMWARE_DIR}" || \
		PrintAndDie "Firmware config files was not found: ${CONF_FIRMWARE_DIR}"
	case "$SUBPROG_ARG" in
		'' ) FirmwareList;;
		*  ) FirmwareCreate "$SUBPROG_ARG.conf";;
	esac
}
