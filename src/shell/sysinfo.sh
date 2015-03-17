WWW_ROOT='/var/www/data/'

BOARD_NAME=$(cat /proc/cpuinfo | grep -E 'Hardware(.*):(.*)' | sed -r 's/(.+): (.+)/\2/g') #'
BOARD_ID=$(cat /proc/cmdline | grep -Eo 'board=([0-9\.]+)' | sed -r 's/board=//g')
HW_ADDR=$(cat /etc/hwaddr)
HW_DDR=$(cat /proc/cpu/clocks | grep ddr | grep -Eo '([0-9]+)MHz')
HW_CPU=$(cat /proc/cpu/clocks | grep 'pll1_main' | grep -Eo '([0-9]+)MHz')
REGIGRAF_PKG=$(ipkg-cl list_installed | grep regigraf)
REGIGRAF_VER=$(echo $REGIGRAF_PKG | grep -Eo '([0-9\.\-]{5,})')
REGIGRAF_INITD=$(ls -l /etc/init.d/S39gui 2> /dev/null)
REGIGRAF_BIN=$(ipkg-cl files regigraf | grep -Eo '^(.+)/main$')
REGIGRAF_NAME=$(cat ${REGIGRAF_BIN/main}/device.xml | grep -Eo 'name="([^"]+)"' | sed -r 's/(.+)"(.+)"/\2/')
KERNEL_HR_TIMER=$(zcat /proc/config.gz | grep CONFIG_HIGH_RES_TIMERS=y)

BOARD_ID=${BOARD_ID:='unknown'}

Kernel() {
  zcat /proc/config.gz | grep "CONFIG_${1}=y" > /dev/null && echo "${1}"
}

