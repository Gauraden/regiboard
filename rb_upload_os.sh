#!/bin/bash

TTY_USB=${1:-/dev/ttyUSB0}
RECIPE=${2:-"RecipeBootOverEth"}
REGILOADER="bin/regiloader"
IMG_UBOOT="output/uboot/u-boot.regigraf.imx53.imx"
IMG_KERNEL="output/kernel/uImage.regigraf.imx53.bin"
IMG_ROOTFS="output/rootfs/rootfs.regigraf.cortex_a8.tar"

HOST_IP=$(ifconfig | grep -oE "inet 192.([0-9\.]+)")
HOST_IP=${HOST_IP/inet /}

RUN="$REGILOADER --tty ${TTY_USB} --img ${IMG_UBOOT} --kernel ${IMG_KERNEL} 
--tftp ${HOST_IP} --rootfs ${IMG_ROOTFS}"

function RecipeBootloader() {
  echo "загружается только UBoot"
  echo "uboot" | ${RUN}
}

function RecipeBootOverUart() {
  echo "загружается UBoot и ядро Linux через UART"
  echo "uboot->kernel_uart" | ${RUN}
}

function RecipeBootOverEth() {
  echo "загружается UBoot и ядро Linux через ethernet"
  echo "uboot->kernel_eth" | ${RUN}
}

echo    "Host is  : ${HOST_IP}"
echo -n "Recipe is: "

${RECIPE}
