#!/bin/sh

# Service variables
_RUN_ID=$(date +%j%H%M%S)
_BIN_DIR=$PATH
_INC_DIR='/usr/include'
_DEV_NULL='/dev/null'
_PROC_VERSION=$(cat /proc/version)
_WGET_ARGS="-q -t ${WGET_TRIES} --waitretry=${WGET_WAIT} "
_USE_SUDO=''
_SYS_LOCALE=$(locale | grep -m1 -P -o '([a-z]{2,2})\_([A-Z]{2,2})' | grep -P -o '([a-z]+)')

if [ "${_EXPORT_ONLY_FUNCS}" = '' ]; then
	. ./.config
	# Common variables
	WORK_DIR=$PWD
	LOG_DIR="${WORK_DIR}/log"
	TMP_DIR="${WORK_DIR}/tmp"
	BUILD_DIR="${WORK_DIR}/build"
	BOARD_DIR="${WORK_DIR}/board"
	DOWNLOAD_DIR="${WORK_DIR}/dl"
	CONF_DIR="${WORK_DIR}/conf"
	PAK_CONF_DIR="${WORK_DIR}/conf/packets"
	OUTPUT_DIR="${WORK_DIR}/output"
	TOOLCHAIN_DIR="${OUTPUT_DIR}/toolchain"
	BUILDROOT_DIR="${OUTPUT_DIR}/buildroot"
	UBOOT_IMG_DIR="${OUTPUT_DIR}/uboot"
	KERNEL_IMG_DIR="${OUTPUT_DIR}/kernel"
	ROOTFS_IMG_DIR="${OUTPUT_DIR}/rootfs"
	PACKETS_DIR="${OUTPUT_DIR}/packets"
	SRC_ETC_DIR="${WORK_DIR}/src/etc"
	SRC_SHELL_DIR="${WORK_DIR}/src/shell"
	SRC_PATCHES_DIR="${WORK_DIR}/src/patches"
	SRC_FIRMWARE_DIR="${WORK_DIR}/src/firmware"
	SRC_UTILS_DIR="${WORK_DIR}/src/utils"

	GCC_VER=$(echo ${_PROC_VERSION} | awk -F '(' '{print $3}' | grep -P -o '([0-9\.]+)')
	LINUX_DIST=$(echo ${_PROC_VERSION} | awk -F '(' '{print $4}' | grep -P -o '^([a-zA-Z]+)')
fi
# Function for translating english message to system locale by using google
# translate.
#   < message to be translated
TranslateByGoogle() {
	local gt_url='http://translate.google.com/translate_a/t'
	# searching message in cache
	local result=''
	local delimiter=']---<>---['
	local cache_file="${TMP_DIR}/msg_cache.${_SYS_LOCALE}"
	if [ -f "$cache_file" ]; then
		result=$(cat "$cache_file" | grep "$1" | awk -F $delimiter '{print $2}')
	else
		# sending request to google
		result=$(wget -U 'Mozilla/5.0' -qO - "$gt_url?client=t&text=$1&sl=auto&tl=${_SYS_LOCALE}" |
		         sed 's/\[\[\[\"//' | cut -d \" -f 1)
		# saving message to cache
		echo "$1${delimiter}${result}" >> $cache_file
	fi
	if [ "$result" = "" ]; then
		echo "$1"
		return 1
	fi
	echo "$result"
}

UseSudo() {
	_USE_SUDO='sudo'
}
# Function for notifying user about finishing of work
#   < message
NotifyUser() {
	if [ "$1" = '' ]; then
		return 0
	fi
	notify-send -u critical "$1"
}

IsItFunction() {
	type -t "$1" > ${_DEV_NULL}
	return $?
}
# Function for printing color message.
#   < message
#   < color
Print() {
  TEXT_COLOR="\033[1;36m" # Cyan
  RESET_COLOR="\033[0m"   # reset esc sequence
	if [ "$2" != "" ]; then
		TEXT_COLOR=$2
	fi
	local msg="$1"
	if [ "${TRANSLATING_MESSAGES}" = 'y' ]; then
		msg=$(TranslateByGoogle "$1")
	fi
	echo -e "${TEXT_COLOR}${3}${msg}${RESET_COLOR}"
}
# Function for printing notifying message.
#   < message
PrintNotice() {
	Print "$1" '\033[1;32m' ' * ' # Green
}
# Function for printing warning message.
#   < message
PrintWarn() {
	Print "$1" '\033[1;33m' ' ! ' # Yellow
}
# Function for printing error message.
#   < message
PrintErr() {
	Print "$1" '\033[1;31m' ' # ' # Red
}
# Function for printing error message and stop the program.
#   < message
PrintAndDie() {
	PrintErr "$1"
	exit 1
}
# Function for printing last messages from log file and additional error
# message, if it nessesary.
# It will stop program execution!
#   < log file name
#   < error message
PrintErrLogAndDie() {
	if [ "$2" != "" ]; then
		PrintErr "$2"
		NotifyUser "Error: $2"
	fi
	tail -n 50 "$1"
	local errors=$(cat "$1" | grep -i 'error')
	if [ "$errors" != "" ]; then
		PrintWarn "$errors"
	fi
	exit 1
}
# Function for executing command and collecting errors to log file.
#   < command to execute
#   < log file name
#   < message for error (not mandatory)
LogIt() {
	$1 2> "$2" || PrintErrLogAndDie "$2" "$3"
}
# Function for checking of directory or file existance.
#   < path to file or directory
#   > result of check
IsFileExists() {
	if [ "$(ls -a -d $1 2> ${_DEV_NULL})" = "" ]; then
		return 1
	fi
	return 0
}
# Function for checking of defining variable.
# If variable is not defined it will stop program execution!
#   < variable value
#   < message to describe variable
DieIfNotDefined() {
	if [ "$1" = "" ]; then
		PrintAndDie "Variable is not defined: $2"
	fi	
}
# Function for creating directory if it's not exists
#   < path
CreateDirIfNotExists() {
	DieIfNotDefined "$1" "name of directory"
	if ! IsFileExists "$1"; then
		mkdir "$1" || PrintAndDie "Failed to create dir: $1"
	fi
}
# Function for making symlink to directory.
#   < source directory
#   < symlink path
BindDirIfRequired() {
	if ! IsFileExists "$1"; then
		PrintAndDie "Source dir was not found: $1"
	fi
	if IsFileExists "$2"; then
		return 0
	fi
	ln -s "$1" "$2"
}
# Function for creating symlink to directory, if source directory is defined.
# If source directory is not defined, it will create an empty one.
#   < destination path (symlink path)
#   < source directory
BindOrCreateDir() {
	if [ "$2" = "" -o "$2" = "/" ]; then
		CreateDirIfNotExists "$1"
		return 0
	fi
	BindDirIfRequired "$2" "$1"
}
# Function for removing symlink.
# It removes only symlinks, ignoring normal files and directories.
#   < path to symlink
RemoveIfSymLink() {
	DieIfNotDefined "$1" "name of sym.link"
	if [ -h "$1" ]; then
		rm "$1"
	fi
}
# Function for replacing content of string using regexp.
#   < string
#   < regexp template
#   < value for replacing
#   > resulting string
Replace() {
	echo "$1" | sed -r "s/$2/$3/g"
}
# Function for extracting parts of path: directory name, file name
#   < path part: file, dir
#   < path value
#   > result of extracting
GetPathPart() {
	if [ "$1" = 'file' ]; then
		echo "$2" | sed -r 's/(.*)\/([^\/]+)$/\2/g'
		return
	fi
	echo "$2" | sed -r 's/(.*)\/([^\/]+)$/\1/g'
}
# Function for checking arguments of config edition methods.
# If config file is not existing, it  will stop program execution!
# Also it will wrap the value by quotes, if length of value is more than 1 symbol!
#   < path to config
#   < name of variable
#   < value for variable
CheckConfigBeforeEdit() {
	DieIfNotDefined "$1" "path to config"
	DieIfNotDefined "$2" "name of variable"
	DieIfNotDefined "$3" "value of variable"
	if ! IsFileExists "$1"; then
		PrintAndDie "Configuration file was not found: $1"
	fi
	CONFIG_VAR_VAL="$3"
	if [ ${#CONFIG_VAR_VAL} != 1 ]; then
		CONFIG_VAR_VAL="\\\"$3\\\""
	fi
}
# Function for adding new variable to config.
#   < path to config
#   < name of variable
#   < value for variable
#   < name of variable after wich, the new one will be placed
AddConfigVarAfter() {
	CheckConfigBeforeEdit $1 $2 $3
	DieIfNotDefined "$4" "name of variable"
	sed -r -e "s:^$4=(.+)$:$4=\1\n$2=${CONFIG_VAR_VAL}:" $1 > "$1.tmp"
	mv "$1.tmp" "$1"
}
# Function for uncommenting already existing variable.
#   < path to config
#   < name of variable
#   < value for variable
DefineConfigVar() {
	CheckConfigBeforeEdit $1 $2 $3
	sed -r -e "s:^# $2 is not set(.*)$:$2=${CONFIG_VAR_VAL}:" $1 > "$1.tmp"
	mv "$1.tmp" "$1"
}
# Function for commenting already existing variable.
#   < path to config
#   < name of variable
UndefineConfigVar() {
	CheckConfigBeforeEdit $1 $2 ' '
	sed -r -e "s:^$2([^=]*)=(.+)$:# $2\1 is not set:" $1 > "$1.tmp"
	mv "$1.tmp" "$1"
}
# Function for changing value of config variable.
#   < path to config
#   < name of variable
#   < new value for variable
SetConfigVar() {
	CheckConfigBeforeEdit $1 $2 $3
	sed -r -e "s:^$2=(.+)$:$2=${CONFIG_VAR_VAL}:" $1 > "$1.tmp"
	mv "$1.tmp" "$1"
}
# Function for retrieving list of directories with executable files.
# It will returns a list of paths for searching of defined file.
#   < name of file wich we want to find
#   > string with a list of paths to searchable file, separated by char ':'
GetBinDirList() {
	echo $(Replace ${_BIN_DIR} '\:' "\/$1\x20")
}
# Function for checking existance of defined util.
# It will search a file with defined name, in all known directories.
# IF file is not found, it will stop program execution!
#   < name of file
DieIfNoTool() {
	if IsFileExists "$(GetBinDirList $1)"; then
		return 0
	fi
	PrintAndDie "Tool was not found: $1"
}
# Function for retrieving name of file from URL.
#   < URL
GetFileNameFromUrl() {
	echo "$1" | grep -P -o "([^\/]+)$"
}
# Function for retrieving size of file defined by URL.
# Not ready!
#   < URL
#   > size of file
GetSizeOfFileByUrl() {
	DieIfNoTool 'wget'
# TODO
#	wget "$1" --spider --server-response -O - 2>&1 | sed -ne '/Content-Length/{s/.*: //;p}' || echo 0
}
# Function for downloading file by URL.
# It uses wget. So the execution of program will be stoped, if no installed wget.
#   < URL for downloading file
#   < directory for saving downloaded file
GetFileByUrl() {
	DieIfNoTool 'wget'
	DieIfNotDefined "$2" "destination directory"
	CreateDirIfNotExists "$2"
	FILE_NAME=$(GetFileNameFromUrl "$1")
	if IsFileExists "$2/${FILE_NAME}"; then
		return 0;
	fi
	PrintNotice "Downloading: ${FILE_NAME} ..."
	wget ${_WGET_ARGS} -O "$2/${FILE_NAME}" "$1" || PrintAndDie "Failed to download file: $1"
	return 0
}
# Function for downloading Git project.
# Subdirectory for project sources will be automaticaly created!
#   < URL to git project
#   < directory for saving project
AttachGitProject() {
	DieIfNoTool 'git'
	DieIfNotDefined "$1" "URL to git project"
	DieIfNotDefined "$2" "directory for storing git project"
	local proj_name=$(echo "$1" | grep -P -o "([^\/]+)$" | awk -F '.git' '{print $1}')
	if ! IsFileExists "$2/${proj_name}"; then
		PrintNotice "Attaching Git project \"${proj_name}\"..."
		cd "$2"
		git clone "$1"
	fi
	PrintNotice "Updating Git project \"${proj_name}\"..."
	cd "$2/${proj_name}"
	git pull
}
# Function for retrieving of installed packet version.
# It uses pkg-config util!
#   < packet name
PkgConfig() {
	if ! IsFileExists "$(GetBinDirList 'pkg-config')"; then
		return 1;
	fi
	pkg-config --modversion $1 2> ${_DEV_NULL} | sed -r "s/\.//g"
}
# Function for checking version of executable file.
# It try to parse the output of "--version"(-V) command!
#   < file name
#   < minimal available version
#   > failed if version < minimal
IsVersion() {
	DieIfNoTool "$1"
	SYS_VER=$(PkgConfig "$1")
	if [ "${SYS_VER}" = "" ]; then
		SYS_VER=$($1 -V | grep -P -o "\d+\..+$" | sed -r "s/\.//g")
	fi
	CMP_VER=$(echo "$2" | sed -r "s/\.//g")
	if [ "$(echo ${SYS_VER}'>='${CMP_VER} | bc -l)" = "1" ]; then
		return 0
	fi
	return 1
}
# Function for creating patch file.
# It's searching "*.orig" files and saving difference between them and new files.
# Result of function is patch file in the TMP_DIR. It will have name of
# directory where was search.
#   < directory for searching "*.orig" files
CreatePatch() {
  DieIfNotDefined "$1" "directory for *.orig files search"
	local patch_name=$(GetPathPart 'file' "$1")
  local patch_file="${TMP_DIR}/$patch_name.r${_RUN_ID}.patch"
  PrintNotice "Creating patch \"$patch_name\"..."
  rm -f "$patch_file"
	for src_file in $(find "$1" -name "*.orig" | sed -r 's/(.*)\.orig$/\1/'); do
		src_file=$(echo "$src_file" | sed -r "s:${WORK_DIR}(.*):\1:g")
		diff -urN ".$src_file.orig" ".$src_file" >> "$patch_file"
	done
	mv "$patch_file" "$SRC_PATCHES_DIR/" || PrintAndDie 'Error while moving patch file!'
}
# Function for applying patch for some sources of code.
# To create patch you can use command: diff -urN olddir newdir
#   < directory of code sources, for patching
#   < patch file name (diff file)
ApplyPatch() {
  if ! IsFileExists "${SRC_PATCHES_DIR}"; then
    PrintAndDie "Directory with patches was not found!"
  fi
  DieIfNotDefined "$1" "directory of patch destination"
  DieIfNotDefined "$2" "diff file"
  PrintWarn "Applying patch \"$2\" to: $1"
  $USE_SUDO patch -d "$1" -p3 < "${SRC_PATCHES_DIR}/$2"
 	USE_SUDO=''
}
# Function for applying group of patches for some sources.
#   < prefix of patches group (PREFIX*****.patch)
#   < directory of code sources, for patching
ApplyAllPatchesFor() {
	PrintNotice "Applying patches ($1*) from: ${SRC_PATCHES_DIR}"
  if ! IsFileExists "${SRC_PATCHES_DIR}"; then
    PrintAndDie "Directory with patches was not found!"
  fi
  local diff_file=''
  for diff_file in $(ls "${SRC_PATCHES_DIR}"); do
  	local is_equal=$(echo $diff_file | grep $1)
		if [ "$is_equal" != "" ]; then
			ApplyPatch "$2" "$diff_file"
		fi
  done
}

GetMountPointOfDevice() {
	DieIfNotDefined "$1" "device name"
	echo $(mount | grep -P -o "^$1 on (.+) type $2" | sed -r "s;(.+) on (.+) type(.+);\2;g")
}

DoYouWantContinue() {
	PrintWarn "$1 y/N:"
	read ANSWER
	if [ "${ANSWER}" = "y" ]; then
		return 0
	fi
	return 1
}
# Function for backuping config
#   < path to the config file
BackupConfig() {
	cp "$1" "$1.backup" 2> ${_DEV_NULL}
}
# Function for swaping config files
#   < destination of config file
#   < config file name
SwapConfig() {
	local config_swap="$1/.config.swap"
	if IsFileExists "${config_swap}"; then
		BackupConfig "$2"
		mv "$1/.config" "$2"
		mv "${config_swap}" "$1/.config"
		return 0
	fi
	mv "$1/.config" "${config_swap}" 2> ${_DEV_NULL}
	cp "$2" "$1/.config" 2> ${_DEV_NULL}
}
# Function for canceling swap operation for config.
#   < directory for searching "swaped" files
CancelSwapConfig() {
	if IsFileExists "$1/.config.swap"; then
		PrintWarn "Canceling of swaped configuration in: $1"
		mv "$1/.config.swap" "$1/.config" 2> ${_DEV_NULL}
	fi
}

ListAllConfigs() {
	PrintNotice "Available configurations:"
	ls -1 "${BOARD_DIR}"
}

LoadBoardConfig() {
	BOARD_CONFIG="$1"
	if [ -f "${BOARD_DIR}/${BOARD_CONFIG}" ]; then
		Print "Loading configuration: ${BOARD_CONFIG} ..."
		. ${BOARD_DIR}/${BOARD_CONFIG}
		return
	fi
	PrintErr "Unknown configuration: ${BOARD_CONFIG}"
	ListAllConfigs
	exit 1
}

UnpackArchive() {
	DieIfNoTool 'tar'
	local dst_dir=${TMP_DIR}
	if [ "$2" != "" ]; then
		dst_dir="$2"
	fi
	PrintNotice "Extracting \"$1\" to: ${dst_dir}"
	if [ -d $1 ]; then
	  PrintWarn "Directory is detected, it will be copied..."
	  ${_USE_SUDO} cp -rP $1 $dst_dir
	else
		${_USE_SUDO} tar -C "${dst_dir}" -xaf "$1"
	fi
	_USE_SUDO=''
}

InstallPacket() {
	if IsFileExists "$(GetBinDirList $1)"; then
		return 0
	fi
	PrintNotice "Installing packet: $1 ..."
	case "${LINUX_DIST}" in
		'Ubuntu' ) sudo apt-get install "$1";;
		'Gentoo' ) PrintWarn "You should manualy install: $1";; #sudo emerge "$1";;
		*        ) PrintAndDie "Unknown OS: ${LINUX_DIST}";;
	esac
	return 0
}

