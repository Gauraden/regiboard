#!/bin/sh

ResetFirmwareConfig() {
  unset FIRMWARE_NAME
  unset FIRMWARE_BOARD
  unset FIRMWARE_IPK
  unset FIRMWARE_FILES
  unset FIRMWARE_SIZE_KB
  unset FIRMWARE_CRC
}

ConfigurateFirmware() {
  FIRMWARE_INFO="${PACKETS_DIR}/${BOARD_NAME}.info"
}

FirmwareList() {
	Print "Доступные прошивки:"
	local firmws=''
	for firmws in $(ls "${CONF_FIRMWARE_DIR}"); do
  	ResetFirmwareConfig
  	. "${CONF_FIRMWARE_DIR}/${firmws}"
		PrintNotice "${firmws/.conf}: плата [$FIRMWARE_BOARD]"
	done
	ResetFirmwareConfig
}

CopyListOfFiles() {
  DieIfNotDefined $3 "директория куда требуется копировать файлы"  
  local file_name=''
  for file_name in $1; do
    # копирование пакета в образ прошивки
    local file_path=${2/\%file_name\%/$file_name}
    local result="[${RED}ошибка   ${MAGENTA}]"
    if IsFileExists "$file_path"; then
      result="[выполнено]"
      sudo cp $file_path $3 > $_DEV_NULL
    fi
    Print "$result $file_name" $MAGENTA ' < '
  done
}

CreatePackagesFile() {
  local ipkg_dir=$1
  local repo_file="${FIRMWARE_INFO}/Packages"
  local tmp_dir="${TMP_DIR}/pkt_info"
  rm -f $repo_file
  mkdir -p $tmp_dir
  cd $tmp_dir
  for pkg in $(ls "$ipkg_dir"); do
    test "${pkg#*.ipk}" != '' && continue
    ar -x "$ipkg_dir/$pkg" control.tar.gz && tar xvf ./control.tar.gz > ${_DEV_NULL}
    cat ./control >> $repo_file
  done
  rm -rf $tmp_dir
}

FirmwareRebuildPackets() {
  local file_name=''
  for file_name in $1; do
    PacketBuild "${file_name}.conf"
  done
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
  # сборка требуемых пакетов
  PrintNotice "Сборка пакетов:"
  FirmwareRebuildPackets "$FIRMWARE_REBUILD"
  # копирование пакетов
  PrintNotice "Копирование пакетов:"
  rm -f "${PACKETS_DIR}/${FIRMWARE_REBUILD}.ipk"
  mv "${PACKETS_DIR}/${FIRMWARE_NAME}.ipk" "${PACKETS_DIR}/${FIRMWARE_REBUILD}.ipk"
  CopyListOfFiles "$FIRMWARE_IPK" "${PACKETS_DIR}/%file_name%.ipk" "$tmp_dir"
  # создание файла-репозитория
  PrintNotice "Создание файла-репозитория..."
  CreatePackagesFile "$tmp_dir"
  # копирование файлов с метаданными прошивки
  PrintNotice "Копирование метаданных..."
  sudo cp ${FIRMWARE_INFO}/${FIRMWARE_NAME}.inf $tmp_dir/${BOARD_NAME}.meta
  sudo cp ${FIRMWARE_INFO}/Packages $tmp_dir
  # упаковка
  PrintNotice "Упаковка образа..."
  sync
  sudo umount $tmp_dir
  sudo losetup -d /dev/$loop_dev
  # запись метаданных и обфусикация
  $UTIL_PROTECTOR --sign ${FIRMWARE_INFO}/${FIRMWARE_NAME}.inf --file $firm_file
  # установка контрольной суммы
  $UTIL_CRC --file $firm_file --crc_value ${FIRMWARE_CRC} --threads 2
}

BuildFirmware() {
  IsFileExists "${CONF_FIRMWARE_DIR}" || \
		PrintAndDie "Firmware config files was not found: ${CONF_FIRMWARE_DIR}"
	case "$SUBPROG_ARG" in
		'') FirmwareList;;
		* ) FirmwareCreate "$SUBPROG_ARG.conf";;
	esac
}
