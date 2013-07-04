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
	local mtd=$(cat /proc/mtd | grep $1 | grep -E -o '^([^\:]+)')
	if ! IsFileExists "/dev/$mtd"; then
		PrintAndDie "MTD with label '$1' was not found!"
	fi
	echo $mtd
}
