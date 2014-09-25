#!/bin/sh
. /usr/lib/regisetup/def.sh
# BOOT_CFG1 --------------------
# 0x38: 0011 1000
# [7:4] : 0011 - Serial ROM
# [ 3 ] : 1    - SPI
echo "0x80c 0x38" > /dev/mxc_iim
# BOOT_CFG2 --------------------
# 0x20: 0010 0000
# [5] : 1 - Размер адреса 24 bit
echo "0x810 0x20" > /dev/mxc_iim
# BOOT_CFG3 --------------------
# 0x20: 0010 0000
# [5:4] : 10 - I2C-3/CSPI
# [3:2] : 00 - CS#0
echo "0x814 0x20" > /dev/mxc_iim
# BOOT_LOCK --------------------
# 0x10: 0001 0000
# [4] : 1 - Чтение конфигурации с FUSE
echo "0x804 0x10" > /dev/mxc_iim
