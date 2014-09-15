#!/bin/sh

# Service variables
_RUN_ID=$(date +%Y%j%H%M%S)
_BIN_DIR=$PATH
_INC_DIR='/usr/include'
_DEV_NULL='/dev/null'
_PROC_VERSION=$(cat /proc/version)
_WGET_ARGS="-q -t ${WGET_TRIES} --waitretry=${WGET_WAIT} "
_USE_SUDO=''
_SYS_LOCALE=$(locale | grep -m1 -P -o '([a-z]{2,2})\_([A-Z]{2,2})' | grep -P -o '([a-z]+)')
_HOST_ARCH=$(arch)

if [ "${_EXPORT_ONLY_FUNCS}" = '' ]; then
	. ./.config
	# Common variables
	WORK_DIR=$PWD
	BIN_DIR="${WORK_DIR}/bin"
	LOG_DIR="${WORK_DIR}/log"
	TMP_DIR="${WORK_DIR}/tmp"
	BUILD_DIR="${WORK_DIR}/build"
	BOARD_DIR="${WORK_DIR}/board"
	DOWNLOAD_DIR="${WORK_DIR}/dl"
	CONF_DIR="${WORK_DIR}/conf"
	CONF_PAK_DIR="${WORK_DIR}/conf/packets"
	CONF_FIRMWARE_DIR="${WORK_DIR}/conf/firmware"
	OUTPUT_DIR="${WORK_DIR}/output"
	TOOLCHAIN_DIR="${OUTPUT_DIR}/toolchain"
	BUILDROOT_DIR="${OUTPUT_DIR}/buildroot"
	UBOOT_IMG_DIR="${OUTPUT_DIR}/uboot"
	KERNEL_IMG_DIR="${OUTPUT_DIR}/kernel"
	ROOTFS_IMG_DIR="${OUTPUT_DIR}/rootfs"
	PACKETS_DIR="${OUTPUT_DIR}/packets"
	INCLUDE_DIR="${OUTPUT_DIR}/include"
  FIRMWARE_DIR="${OUTPUT_DIR}/firmware"
	SRC_ETC_DIR="${WORK_DIR}/src/etc"
	SRC_SHELL_DIR="${WORK_DIR}/src/shell"
	SRC_PATCHES_DIR="${WORK_DIR}/src/patches"
	SRC_FIRMWARE_DIR="${WORK_DIR}/src/firmware"
	SRC_UTILS_DIR="${WORK_DIR}/src/utils"

	GCC_VER=$(echo ${_PROC_VERSION} | awk -F '(' '{print $3}' | grep -P -o '([0-9\.]+)')
	LINUX_DIST=$(echo ${_PROC_VERSION} | awk -F '(' '{print $4}' | grep -P -o '^([a-zA-Z]+)')
fi

CYAN='\033[1;36m'
MAGENTA='\033[1;35m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
RED='\033[1;31m'
GREY='\033[1;30m'
BLUE='\033[1;34m'
FONT_BOLD='\033[1m'
FONT_UNDER='\033[4m'
ESC_RESET='\033[0m'

# Function for cheking variable definition.
#   < value of variable
IsDefined() {
	if [ "$1" = "" ]; then
    return 1
	fi	
	return 0
}
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
	IsDefined "$result" && echo "$result" && return 0
	echo "$1"
	return 1
}
# Function for enabling SUDO mode for following calls
UseSudo() {
	_USE_SUDO='sudo'
}
# Function for notifying user about finishing of work
#   < message
NotifyUser() {
  test "$TERM" == 'xterm' && whereis -b $TERM | grep -q '/.*' || return 0
 	IsDefined "$1" && notify-send -u critical "$1"
}
# Function for executing command and skipping error output
#   < string with command
IgnoreErrOut() {
  IsDefined "$1" && $1 2> ${_DEV_NULL} && return 0
  return 1
}
# Function for executing command and skipping all output
#   < string with command
IgnoreAllOut() {
  IsDefined "$1" && $1 > ${_DEV_NULL} 2> ${_DEV_NULL} && return 0
  return 1
}
# Function for checking existence of function, which name was passed through
# argument.
#   < name of function
#   > 0(true, it means no error) if function is existed; !=0(false, it means
#     any error)
IsItFunction() {
	type -t "$1" > ${_DEV_NULL}
	return $?
}
# Function for printing color message.
#   < message
#   < color
Print() {
  TEXT_COLOR=$CYAN
  RESET_COLOR=$ESC_RESET
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
	Print "$1" $GREEN ' * '
}
# Function for printing warning message.
#   < message
PrintWarn() {
	Print "$1" $YELLOW ' ! '
}
# Function for printing error message.
#   < message
PrintErr() {
	Print "$1" $RED ' # '
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
  IsDefined "$2" && PrintErr "$2" && NotifyUser "Error: $2"
	tail -n 50 "$1"
	local errors=$(cat "$1" | grep -i 'error')
	IsDefined "$errors" && PrintWarn "$errors"
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
  IsDefined "$(ls -a -d $1 2> ${_DEV_NULL})" && return 0
  return 1
}
# Function for checking of defining variable.
# If variable is not defined it will stop program execution!
#   < variable value
#   < message to describe variable
DieIfNotDefined() {
  IsDefined "$1" || PrintAndDie "Variable is not defined: $2"
}
# Function for creating directory if it's not exists
#   < path
CreateDirIfNotExists() {
	DieIfNotDefined "$1" "name of directory"
	IsFileExists "$1" || mkdir -p "$1" || PrintAndDie "Failed to create dir: $1"
}
# Function for moving content of one directory to another
#   < path to the directory, content of which need to be moved
#   < path to the directory, for saving content
MoveDir() {
  DieIfNotDefined "$1" "name of source directory"
  DieIfNotDefined "$2" "new name for directory"
  CreateDirIfNotExists "$2"
  IsFileExists "$1" || return 0
  cp -ru $1/* $2/
  rm -rf "$1"
}
# Function for making symlink to directory.
#   < source directory
#   < symlink path
BindDirIfRequired() {
	IsFileExists "$1" || PrintAndDie "Source dir was not found: $1"
	IsFileExists "$2" && return 0
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
	DieIfNotDefined "$1" "name of symlink"
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
	DieIfNotDefined "$2" "name of variable in \"$1\""
	DieIfNotDefined "$3" "value of variable \"$2\""
	IsFileExists "$1" || PrintAndDie "Configuration file was not found: $1"
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
	IsFileExists "$(GetBinDirList $1)" || PrintAndDie "Tool was not found: $1"
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
	IsFileExists "$2/${FILE_NAME}" && return 0;
	PrintNotice "Downloading: ${FILE_NAME} ..."
	wget ${_WGET_ARGS} -O "$2/${FILE_NAME}" "$1" || \
	  PrintAndDie "Failed to download file: $1"
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
	IsFileExists "$(GetBinDirList 'pkg-config')" || return 1;
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
		diff -Nur ".$src_file.orig" ".$src_file" >> "$patch_file"
	done
	mv "$patch_file" "$SRC_PATCHES_DIR/" || \
	  PrintAndDie 'Error while moving patch file!'
}
# Function for applying patch for some sources of code.
# To create patch you can use command: diff -urN olddir newdir
#   < directory of code sources, for patching
#   < patch file name (diff file)
ApplyPatch() {
  IsFileExists "${SRC_PATCHES_DIR}" || \
    PrintAndDie "Directory with patches was not found!"
  DieIfNotDefined "$1" "directory of patch destination"
  DieIfNotDefined "$2" "diff file"
  PrintWarn "Applying patch \"$2\" to: $1"
  $USE_SUDO patch -N -d "$1" -p3 < "${SRC_PATCHES_DIR}/$2"
 	USE_SUDO=''
}
# Function for applying group of patches for some sources.
#   < prefix of patches group (PREFIX*****.patch)
#   < directory of code sources, for patching
ApplyAllPatchesFor() {
	PrintNotice "Applying patches ($1*) from: ${SRC_PATCHES_DIR}"
  IsFileExists "${SRC_PATCHES_DIR}" || \
    PrintAndDie "Directory with patches was not found!"
  local diff_file=''
  for diff_file in $(ls "${SRC_PATCHES_DIR}"); do
  	local is_equal=$(echo $diff_file | grep $1)
		IsDefined $is_equal && ApplyPatch "$2" "$diff_file"
  done
}
# Function for getting name of device mount point
#   < name of device (/dev/...)
GetMountPointOfDevice() {
	DieIfNotDefined "$1" "device name"
	echo $(mount | grep -P -o "^$1 on (.+) type $2" | \
	  sed -r "s;(.+) on (.+) type(.+);\2;g")
}
# Function for asking user a question, something like modal dialog (Yes/No)
#   < question text
#   > 0(true) user answer is "Yes"; !=0(false) answer is "No"
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
  IsFileExists "$1/.config.swap" || return 0
	PrintWarn "Canceling of swaped configuration in: $1"
	mv "$1/.config.swap" "$1/.config" 2> ${_DEV_NULL}
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
		TARGET_NAME_CPU="${BOARD_NAME}.${BOARD_CPU}"
		TARGET_NAME_PREFIX="${BOARD_NAME}.${BOARD_PREFIX}"
		return
	fi
	PrintErr "Unknown configuration: ${BOARD_CONFIG}"
#	ListAllConfigs
	return 1
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
	  ${_USE_SUDO} rsync -ura --exclude='.git' --exclude='.gitignore' $1 $dst_dir
	else
		${_USE_SUDO} tar -C "${dst_dir}" -xaf "$1"
	fi
	_USE_SUDO=''
}

InstallPacket() {
  whereis -b $1 | grep -q 'bin/.*' && return 0
	PrintNotice "Installing packet: $1 ..."
	case "${LINUX_DIST}" in
		'Ubuntu' ) sudo apt-get install "$1";;
		'Gentoo' ) PrintWarn "You should manualy install: $1";; #sudo emerge "$1";;
		*        ) PrintAndDie "Unknown OS: ${LINUX_DIST}";;
	esac
	return 0
}

GetUserFromUrl() {
  echo "$1" | sed -r 's/^([^@]+)@(.+)$/\1/'
}

GetHostFromUrl() {
  echo "$1" | sed -r 's/^([^@]+)@(.+)$/\2/'
}

GetFtpDir() {
  local ftp_url="ftp://${REPO_URL_FTP}/${1}"
  PrintNotice "Downloading files from: $ftp_url -> $2"
  cd $2
  wget -r -nH --cut-dirs=1 ${ftp_url}/*
}

GetFtpDirList() {
  cd $TMP_DIR
  wget --spider --no-remove-listing --password=$REPO_PSW_FTP \
    ftp://${REPO_URL_FTP}/${1}/
  echo "${TMP_DIR}/.listing"
}

UploadDataToRemoteRepo() {
	DieIfNoTool 'ncftp'
	PrintNotice "Uploading \"$1\" to: $REPO_URL_FTP/$2"
	local user=$(GetUserFromUrl $REPO_URL_FTP)
	local host=$(GetHostFromUrl $REPO_URL_FTP)
	ncftpput -u$user -p"$REPO_PSW_FTP" -C "$host" $1 "/$2" || \
	  PrintWarn "Failed to upload file: $1"
}

FindUSBDevice() {
  lsusb | grep "$1" > /dev/null && return 0
  return 1
}

PrintWithColors() {
  local header_1="s/=([^=]+)=/\\${MAGENTA}\1\\${ESC_RESET}/g"
  local header_2="s/==([^=]+)==/\\${CYAN}\1\\${ESC_RESET}/g"
  local option_lst="s/\*([^:]+):/\\${GREEN}\*\1:\\${ESC_RESET}/g"
  local variable="s/<([^>]+)>/\\${YELLOW}<\1>\\${ESC_RESET}/g"
  local example="s/:([^;]+);/:\\${FONT_BOLD}\1\\${ESC_RESET};/g"
  local dquoted="s/\"([^\"]+)\"/\"\\${BLUE}\1\\${ESC_RESET}\"/g"
  local quoted="s/\'([^\']+)\'/\'${BLUE}\1${ESC_RESET}\'/g"
  for line in "$(cat $1)"; do
    line=$(echo "$line" | sed -r $header_2)
    line=$(echo "$line" | sed -r $header_1)
    line=$(echo "$line" | sed -r $option_lst)
    line=$(echo "$line" | sed -r $example)
    line=$(echo "$line" | sed -r $variable)
    line=$(echo "$line" | sed -r $dquoted)
    line=$(echo "$line" | sed -r $quoted)
    echo -e "$line"
  done
}
