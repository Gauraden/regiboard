#!/bin/sh

. ./rb_functions.sh

CMD=$1
ARG=$2
BLOCK_SIZE=255
TEST_ODATA='./ttyusb_odata.bin'
TEST_IDATA='./ttyusb_idata.bin'
LET=bash -c let

if ! IsFileExists '/bin/bash'; then
	LET=let
fi

PrepareDataForTransfer() {
	Print "Preparing data for transfer: $TEST_ODATA"
	local count=0
	while [ $count -le $BLOCK_SIZE ];	do
		echo 'F' >> $TEST_ODATA
		"$LET count++"
	done
}

SetBaud() {
	local tty=$1
	local baud=$2
	local res=$(stty -F /dev/$tty speed $baud 2> ${_DEV_NULL})
	if [ $res = '' ]; then
		return 1
	fi
	return 0
}

GetAvailableBaud() {
	local tty=$1
	local baud=115200
	local prev_baud=$(stty -F /dev/$tty | sed -r 's/speed ([0-9]+) baud(.*)/\1/' \
                                      | grep -P '([0-9]{4,})')
	Print "Scaning for available baud on: $tty"
	PrintNotice "Previous baud: $prev_baud"
	while [ $baud -lt 2000000 ]; do
		if ! SetBaud $tty $baud; then
			return
		fi
		PrintNotice "$baud"
		"$LET baud=baud * 2"
	done
	# восстановим начальное значение
	SetBaud $tty $prev_baud
}

SendingBlock() {
	PrintNotice "Sending block[$BLOCK_SIZE] to: $1"
	dd if=$TEST_ODATA of=/dev/$1 bs=$BLOCK_SIZE count=1
}

RecievingBlock() {
	PrintNotice "Recieving block[$BLOCK_SIZE] from: $1"
	dd if=/dev/$1 of=$TEST_IDATA bs=$BLOCK_SIZE count=1 > ${_DEV_NULL}
	cmp $TEST_IDATA $TEST_ODATA
	rm $TEST_IDATA
}

DoForTTYDevice() {
	Print "Do '$1' for all devices:"
	for tty_dev in $(ls /dev/ | grep -oE 'ttyUSB([0-9]+)'); do
		$1 $tty_dev $2
	done
}

if ! IsFileExists "$TEST_ODATA"; then
	PrepareDataForTransfer
fi

case "$CMD" in
	'send'    ) DoForTTYDevice 'SendingBlock';;
	'recieve' ) DoForTTYDevice 'RecievingBlock';;
	'setbaud' ) DoForTTYDevice 'SetBaud' $ARG;;
	'getbaud' ) GetAvailableBaud $ARG;;
	*         ) PrintWarn "Unknown command: $CMD";;
esac

