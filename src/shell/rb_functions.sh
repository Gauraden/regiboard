#!/bin/sh

# Service variables
_BIN_DIR=${PATH}
_DEV_NULL='/dev/null'

IsItFunction() {
	type -t "$1" > ${_DEV_NULL}
	return $?
}
# Function for printing color message.
#   < message
#   < color
#   < prefix
Print() {
  TEXT_COLOR="\033[1;36m" # Cyan
  RESET_COLOR="\033[0m"   # reset esc sequence
	if [ "$2" != "" ]; then
		TEXT_COLOR=$2
	fi
	local msg="$1"
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
	Print "$1" '\033[1;33m' ' * ' # Yellow
}
# Function for printing error message.
#   < message
PrintErr() {
	Print "$1" '\033[1;31m' ' ! ' # Red
}
# Function for printing error message and stop the program.
#   < message
PrintAndDie() {
	PrintErr "$1"
	exit 1
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
# Function for checking of directory or file existance.
#   < path to file or directory
#   > result of check
IsFileExists() {
	if [ "$(ls -a -d $1 2> ${_DEV_NULL})" = "" ]; then
		return 1
	fi
	return 0
}
# Function for replacing content of string using regexp.
#   < string
#   < regexp template
#   < value for replacing
#   > resulting string
Replace() {
	echo "$1" | sed -r "s/$2/$3/g"
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
# Function return MTD name for defined label.
#   < label
#   > name of MTD
GetMTDFor() {
	DieIfNotDefined 'MTD label'
	local mtd=$(cat /proc/mtd | grep $1 | grep -E -o '^([^\:]+)') #'
	if ! IsFileExists "/dev/$mtd"; then
		PrintAndDie "MTD with label '$1' was not found!"
	fi
	echo $mtd
}
# Function for disabling printing messages of linux kernel on console.
DisableKernelMessages() {
	local printk_cnf='/proc/sys/kernel/printk'
	if ! IsFileExists $printk_cnf; then
		PrintWarn 'Disabling messages of kernel is failed!'
		return 0
	fi
	echo 0 > $printk_cnf
}
# Function for enabling printing messages of linux kernel on console.
EnableKernelMessages() {
	local printk_cnf='/proc/sys/kernel/printk'
	if ! IsFileExists $printk_cnf; then
		PrintWarn 'Enabling messages of kernel is failed!'
		return 0
	fi
	echo 1 > $printk_cnf
}
# Function for extracting bits from bytes.
#   < offset of extracting bits
#   < source value for extraction bits
#   < mask of extracting bits. Not mandatory, by default will use 1!
#   > value representing extracted bits
GetBit() {
	local offs=$1
	local src=$2
	local mask=$3
	if [ "$mask" = "" ]; then
		mask='1'
	fi
	let "BIT_VALUE=($2 >> $offs) & $mask" && echo $BIT_VALUE || echo '0'
}
# Function for reading data from FUSE.
#   < name of FUSE register
#   < offset of FUSE register
#   < handler for value that was extracted from FUSE
ReadFuse() {
	DieIfNotDefined $FUSE_DEV 'Name of FUSE device!'
	local fuse_addr=`echo $(($2))`
	local fuse_val=$(dd if=$FUSE_DEV bs=1 skip=$fuse_addr count=1 2> /dev/null | \
	  hexdump | \
	  head -1 | \
	  sed -r 's/^\w+\ ([0-9a-f]{4,4})/\1/g')
	PrintNotice "FUSE [$2] $1: $fuse_val"
	local fuse_parser=$3
	if [ "$fuse_parser" = "" ]; then
		return;
	fi
	$fuse_parser $fuse_val
}
# Function for downloading files from FTP.
#   < name of downloadable file
#   < name of directory to save this file
GetFileFromFTP() {
	local file_name="$1"
	local output_dir="$2"
	DieIfNotDefined $file_name    'Name of downloadable file!'
	DieIfNotDefined $output_dir   'Name of output directory!'
	DieIfNotDefined $FTP_REPO_URL 'FTP repository URL!'
	DieIfNotDefined $FTP_REPO_PSW 'FTP repository password!'
	IsItFunction 'wget' || PrintAndDie 'No wget util was found!'
	PrintNotice "Downloading..."
	wget --password=$FTP_REPO_PSW -P "$output_dir" ftp://$FTP_REPO_URL/$file_name
}
