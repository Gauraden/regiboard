#!/bin/sh

. /usr/lib/regisetup/hw.sh

DEV_PARSER='ParseUnknownCfg'

if ! IsFileExists $DEV_FUSE; then
  PrintAndDie "FUSE device \"$DEV_FUSE\" was not found!"
fi
# Parsers for configuration of unknown device
ParseUnknownCfg() {
	return 0
}
ParseUnknownCfg2() {
	return 0
}
ParseUnknownCfg3() {
	return 0
}
# Parsers for configuration of NAND
ParseNandCfg() {
	local fuse_val=$1
	local dev_name=$2
	local muxed_on=$(GetBit 6 $fuse_val 0x1)
	local interleave=$(GetBit 4 $fuse_val 0x3)
	local addr_cycle=$(GetBit 2 $fuse_val 0x3)
	local bt_freq=$(GetBit 1 $fuse_val 0x1)
	local mmu_enable=$(GetBit 0 $fuse_val 0x1)
	if [ "$muxed_on" = '0' ]; then
		muxed_on='PATA'
	else
		muxed_on='WEIM'
	fi
	case "$interleave" in
		'0' ) interleave='No';;
		'1' ) interleave='2 device';;
		'2' ) interleave='4 device';;
		'3' ) interleave='Reserved';;
		*   ) interleave='unknown';;
	esac
  addr_cycle=$(($addr_cycle + 3))
	if [ "$bt_freq" = '0' ]; then
		bt_freq='800'
	else
		bt_freq='400'
	fi
	# Printing config
	Print " * Device....: $dev_name"
	Print " * Muxed on..: $muxed_on"
	Print " * Interleave: $interleave"
	Print " * Addr cycle: $addr_cycle"
	Print " * Boot freq.: $bt_freq"
	Print " * MMU enable: $mmu_enable"
}
ParseNandCfg2() {
	local fuse_val=$1
	local page_size=$(GetBit 6 $fuse_val 0x3)
	local nand_if=$(GetBit 5 $fuse_val 0x1)
	local ddr_freq=$(GetBit 4 $fuse_val 0x1)
	local osc_freq=$(GetBit 3 $fuse_val 0x1)
	local nfc_freq=$(GetBit 2 $fuse_val 0x1)
	local sec_conf=$(GetBit 0 $fuse_val 0x3)
	case "$page_size" in
		'0' ) page_size='512 + 16 Bytes (4-bit ECC)';;
		'1' ) page_size='2KB + 64 Bytes';;
		'2' ) page_size='4KB + 128 Bytes';;
		'3' ) page_size='4KB + 218 Bytes';;
		*   ) page_size='unknown';;
	esac
	if [ "$nand_if" = '0' ]; then
		nand_if='8 bit'
	else
		nand_if='16 bit'
	fi
	if [ "$ddr_freq" = '0' ]; then
		ddr_freq='PLL2 - 400MHz'
	else
		ddr_freq='PLL2 - 333MHz'
	fi
	if [ "$osc_freq" = '0' ]; then
		osc_freq='19.2, 24, 26, 27 MHz - auto detect'
	else
		osc_freq='Oscillator frequency is 24 MHz'
	fi
	if [ "$nfc_freq" = '0' ]; then
		nfc_freq='AXI DDR divide by 12'
	else
		nfc_freq='AXI DDR divide by 28'
	fi
	if [ $sec_conf -lt 2 ]; then
		sec_conf='Off'
	else
		sec_conf='On'
	fi
	# Printing config
	Print " * Page size.: $page_size"
	Print " * NAND iface: $nand_if"
	Print " * DDR freq..: $ddr_freq"
	Print " * OSC freq..: $osc_freq"
	Print " * NFC freq..: $nfc_freq"
	Print " * Security..: $sec_conf"
}
ParseNandCfg3() {
	local fuse_val=$1
	local stride_size=$(GetBit 7 $fuse_val 0x1)
	local lba_nand=$(GetBit 6 $fuse_val 0x1)
	local nand_rb=$(GetBit 5 $fuse_val 0x1)
	local ecc=$(GetBit 3 $fuse_val 0x3)
	local pages=$(GetBit 1 $fuse_val 0x3)
	local dir_bt=$(GetBit 0 $fuse_val 0x1)
	if [ "$stride_size" = '0' ]; then
		stride_size='1 Block'
	else
		strude_size='8 Blocks'
	fi
	if [ "$lba_nand" = '0' ]; then
		lba_nand='Non LBA (11ms delay)'
	else
		lba_nand='LBA (22ms delay)'
	fi	
	if [ "$nand_rb" = '0' ]; then
		nand_rb='No'
	else
		nand_rb='Yes'
	fi
	case "$ecc" in
		'0' ) ecc='8-bit';;
		'1' ) ecc='14';;
		'2' ) ecc='16';;
		'3' ) ecc='Off';;
		*   ) ecc='unknown';;
	esac
	case "$pages" in
		'0' ) pages='32';;
		'1' ) pages='64';;
		'2' ) pages='128';;
		'3' ) pages='256';;
		*   ) pages='unknown';;
	esac
	if [ "$dir_bt" = '0' ]; then
		dir_bt='allowed'
	else
		dir_bt='not allowed'
	fi
	# Printing config
	Print " * Stride....: $stride_size"
	Print " * LBA.......: $lba_nand"
	Print " * Use R/B...: $nand_rb"
	Print " * ECC/Spare.: $ecc"
	Print " * Pages.....: $pages"
	Print " * Boot......: direct boot from external memory is $dir_bt"
}
ParseSerialROMCfg() {
	local fuse_val=$1
	local dev_name=$2
	local rom_select=$(GetBit 3 $fuse_val 0x1)
	if [ "$rom_select" = '0' ]; then
	  rom_select='I2C'
	else
	  rom_select='SPI'
	fi
	# Printing config
	Print " * Device....: $dev_name"
	Print " * Serial ROM: $rom_select"
}
ParseSerialROMCfg2() {
	local fuse_val=$1
	local addressing=$(GetBit 5 $fuse_val 0x1)
	if [ "$addressing" = '0' ]; then
  	addressing='16'
	else
	  addressing='24'
	fi
	# Printing config
	Print " * Addressing: $addressing bit"
}
ParseSerialROMCfg3() {
	local fuse_val=$1
	local port_select=$(GetBit 4 $fuse_val 0x3)
	local cs_select=$(GetBit 2 $fuse_val 0x3)
	case "$port_select" in
		'0' ) port_select='I2C-1/ECSPI-1';;
		'1' ) port_select='I2C-2/ECSPI-2';;
		'2' ) port_select='I2C-3/CSPI';;
		'3' ) port_select='Reserved';;
		*   ) port_select='unknown';;
	esac
	# Printing config
	Print " * Port......: $port_select"
	Print " * CS(SPI)...: $cs_select"
  return 0
}
# Parser of BOOT_CFG1 register
ParseBootCFG1() {
	local fuse_val=$1
	local dev_type=$(GetBit 4 $fuse_val 0xf)
	local dev_name='unknown'
	# checking devices of the first group
	case "$dev_type" in
		# WEIM
		'0' ) dev_name='WEIM';;
		# HD (SATA/PATA)
		'2' ) dev_name='HD (SATA/PATA)';;
		# Serial ROM
		'3' ) dev_name='Serial ROM';;
		*   );;
	esac
	# checking devices of the second group
	dev_type=$(($dev_type >> 1))
	case "$dev_type" in
		# SD/eSD
		'2' ) dev_name='SD/eSD';;
		# MMC/eMMC
		'3' ) dev_name='MMC/eMMC';;
		# Unknown
		*   );;
	esac
	# maybe it's NAND
	dev_type=$(($dev_type >> 2))
	if [ "$dev_type" = '1' ]; then
		dev_name='NAND Flash'
	fi
	# ---------------------
	# output of parsed data
	case "$dev_name" in
		'WEIM'           ) ;;
		'HD (SATA/PATA)' ) ;;
		'Serial ROM'     ) DEV_PARSER='ParseSerialROMCfg';;
		'SD/eSD'         ) ;;
		'MMC/eMMC'       ) ;;
		'NAND Flash'     ) DEV_PARSER='ParseNandCfg';;
		*                ) ;;
	esac
	$DEV_PARSER $fuse_val $dev_name
}
# parser of BOOT_LOCK register
ParseBootLock() {
	local fuse_val=$1
	local jtag_smode=$(GetBit 5 $fuse_val 0x3)
	local bt_fuse=$(GetBit 4 $fuse_val 0x1)
	local jtag_heo=$(GetBit 3 $fuse_val 0x1)
	local kte=$(GetBit 2 $fuse_val 0x1)
	local sec_jtag=$(GetBit 1 $fuse_val 0x1)
	local jtag=$(GetBit 0 $fuse_val 0x1)
	case "$jtag_smode" in
		'0' ) jtag_smode='JTAG enable mode';;
		'1' ) jtag_smode='Secure JTAG mode';;
		'3' ) jtag_smode='No debug mode';;
		*   ) jtag_smode='unknown';;
	esac
	if [ "$bt_fuse" = '0' ]; then
		bt_fuse="Boot mode configuration is taken from GPIOs | Boot using Serial Loader"
	else
		bt_fuse='Boot mode configuration is taken from fuses'
	fi
	# Printing config
	Print " * JTAG mode.: $jtag_smode"
	Print " * Boot mode.: $bt_fuse"
}
# Look at iMX53RM.pdf (page 5092). There will be addresses of FUSE registers.
DisableKernelMessages
ReadFuse 'BOOT_CFG1' 0x080C 'ParseBootCFG1'
ReadFuse 'BOOT_CFG2' 0x0810 "${DEV_PARSER}2"
ReadFuse 'BOOT_CFG3' 0x0814 "${DEV_PARSER}3"
ReadFuse 'BT_LPB   ' 0x0818
ReadFuse 'BOOT_LOCK' 0x0804 'ParseBootLock'
EnableKernelMessages
