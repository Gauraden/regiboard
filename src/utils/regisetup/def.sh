#!/bin/sh

. /etc/regisetuprc

_DEV_NULL='/dev/null'
_PRINTK_FLAG='/proc/sys/kernel/printk'

IsAvailable() {
	type -t "$1" > ${_DEV_NULL}
	return $?
}

IsDefined() {
	if [ "$1" = "" ]; then
    return 1
  fi
  return 0
}

Print() {
	local txt="\033[1;36m" # Cyan
	local reset="\033[0m"  # reset esc sequence
	if [ "$2" != "" ]; then
	  txt=$2
	fi
	local msg="$1"
	echo -e "${txt}${3}${msg}${reset}"
}

Notice() {
	Print "$1" '\033[1;32m' ' * ' # Green
}

Warn() {
	Print "$1" '\033[1;33m' ' ! ' # Yellow
}

Error() {
	Print "$1" '\033[1;31m' ' # ' # Red
}

Throw() {
	Error "$1"
	exit 1
}

ThrowUndefined() {
  IsDefined $1 || Throw "Variable is not defined: $2"
}

IsFileExists() {
	IsDefined "$(ls -a -d $1 2> ${_DEV_NULL})" && return 0
	return 1
}

Replace() {
	echo "$1" | sed -r "s/$2/$3/g"
}

ThrowUnavailable() {
	IsAvailable "$1" || Throw "Tool was not found: $1"
}

GetMTDFor() {
	ThrowUndefined 'MTD label'
	local mtd=$(cat /proc/mtd | grep $1 | grep -E -o '^([^\:]+)') #'
	IsFileExists "/dev/$mtd" || Throw "MTD with label '$1' was not found!"
	echo $mtd
}

DisableKernelMessages() {
	IsFileExists ${_PRINTK_FLAG} || \
	  Warn 'Disabling of kernel messages is failed!' && return 1
	echo 0 > ${_PRINTK_FLAG}
}

EnableKernelMessages() {
	IsFileExists ${_PRINTK_FLAG} || \
	  Warn 'Enabling of kernel messages is failed!' && return 1
	echo 1 > ${_PRINTK_FLAG}
}

GetBit() {
	local offs=${1:-'0'}
	local src=${2:-'0'}
	local mask=${3:-'1'}
  local bit=$((($src >> $offs) & $mask))
  test $bit && echo $bit || echo 0
}

GetFileFromFTP() {
	local file_name="$1"
	local output_dir="$2"
	ThrowUndefined $file_name    'Name of downloadable file!'
	ThrowUndefined $output_dir   'Name of output directory!'
	ThrowUndefined $FTP_REPO_URL 'FTP repository URL!'
	ThrowUndefined $FTP_REPO_PSW 'FTP repository password!'
	ThrowUnavailable 'wget'
	Notice "Downloading..."
	wget --password=$FTP_REPO_PSW -P "$output_dir" ftp://$FTP_REPO_URL/$file_name
}
