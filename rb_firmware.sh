#!/bin/sh

ResetFirmwareConfig() {
  unset FIRMWARE_NAME
  unset FIRMWARE_BOARD
  unset FIRMWARE_IPK
  unset FIRMWARE_FILES
  unset FIRMWARE_SIZE_KB
}

FirmwareList() {
	Print "Доступные прошивки:"
	for firmws in $(ls "${CONF_FIRMWARE_DIR}" | sed -r 's:(.+)\.conf:\1:'); do
  	ResetFirmwareConfig
  	. "${CONF_FIRMWARE_DIR}/${firmws}.conf"
		PrintNotice "$firmws: плата [$FIRMWARE_BOARD]"
	done
	ResetFirmwareConfig
}

CopyListOfFiles() {
  DieIfNotDefined $3 "директория куда требуется копировать файлы"  
  for file_name in $1; do
    local file_path=$(echo "$2" | sed -r "s:%file_name%:$file_name:")
    local result="[${RED}ошибка   ${MAGENTA}]"
    if IsFileExists "$file_path"; then
      result="[выполнено]"
      $_USE_SUDO cp $file_path $3 > $_DEV_NULL
    fi
    Print "$result $file_name" $MAGENTA ' < '
  done
  _USE_SUDO=''
}

FirmwareCreate() {
  local firmw_conf=$1
  DieIfNotDefined $firmw_conf "имя конфигурации прошивки"
  IsFileExists "${CONF_FIRMWARE_DIR}/$firmw_conf" || \
    PrintAndDie "конфигурация \"$1\" не найдена"
  ResetFirmwareConfig
  . ${CONF_FIRMWARE_DIR}/$firmw_conf
  Print "Создание прошивки: $FIRMWARE_NAME"
  if [ "$FIRMWARE_BOARD" != "$BOARD_NAME" ]; then
    PrintAndDie "Прошивка не совместима с платой: $FIRMWARE_BOARD != $BOARD_NAME"
  fi
  # подготовка временной директории
  local uid=$(id | grep -Po 'uid=([0-9]+)')
  local loop_dev=loop0
  local fs_type=ext2
  local tmp_dir="${BUILD_DIR}/firmware.${BOARD_PREFIX}.${FIRMWARE_NAME}"
  local firm_file="${FIRMWARE_DIR}/${BOARD_PREFIX}.${FIRMWARE_NAME}.rbf"
  PrintNotice "Подготовка директории образа (${FIRMWARE_SIZE_KB}KB)..."
  dd if=/dev/zero of=$firm_file bs=1024 count=$FIRMWARE_SIZE_KB
  sudo losetup /dev/$loop_dev $firm_file
  sudo mkfs -t $fs_type -m 1 -v /dev/$loop_dev
  CreateDirIfNotExists "$tmp_dir"
  sudo mount -t $fs_type /dev/$loop_dev $tmp_dir
  # копирование системных пакетов
  PrintNotice "Копирование системных пакетов:"
  UseSudo
  CopyListOfFiles "$FIRMWARE_IPK" "${PACKETS_DIR}/%file_name%.ipk" "$tmp_dir"
  # копирование внешних файлов
  PrintNotice "Копирование внешних файлов ($EXT_PROJECT_REGIGRAF):"
  UseSudo
  CopyListOfFiles "$FIRMWARE_FILES" "${EXT_PROJECT_REGIGRAF}/%file_name%" "$tmp_dir"
  # упаковка
  PrintNotice "Упаковка образа..."
  sudo umount $tmp_dir
  sudo losetup -d /dev/$loop_dev
}

BuildFirmware() {
  IsFileExists "${CONF_FIRMWARE_DIR}" || \
		PrintAndDie "Firmware config files was not found: ${CONF_FIRMWARE_DIR}"
	case "$SUBPROG_ARG" in
		'') FirmwareList;;
		* ) FirmwareCreate "$SUBPROG_ARG.conf";;
	esac
}
