#!/bin/bash

. ./.config

RECIPE=${1:-"Default"}
TTY_USB=${2:-/dev/ttyUSB0}
REGILOADER="bin/regiloader"
IMG_UBOOT="output/uboot/u-boot.regigraf.imx53"
IMG_KERNEL="output/kernel/uImage.regigraf.imx53.bin"
IMG_ROOTFS="output/rootfs/rootfs.regigraf.cortex_a8.tar.gz"
IMG_MTD_UTILS="tmp/mtd-utils.ipk/data.tar.gz"
IMG_RBF="cortex_a8.regigraf.1772.53.UNIVERSAL-last.rbf"
IMG_RBF_PATH="tmp/"
IMG_RBF_URL="ftp://jenny/firmwares/F1772/${IMG_RBF}"

HOST_IP=$(ifconfig | grep -oE "inet 192.([0-9\.]+)")
HOST_IP=${HOST_IP/inet /}

RUN="$REGILOADER --tty ${TTY_USB} --img ${IMG_UBOOT} --kernel ${IMG_KERNEL} 
--tftp ${HOST_IP} --rootfs ${IMG_ROOTFS} --utils ${IMG_MTD_UTILS} --conf ./tmp 
--rbf ${IMG_RBF_PATH}${IMG_RBF} --uboot_pswd ${UBOOT_PSWD} ${RBUP_EXT_OPTS}"

function RecipeBootloader() {
  ${RUN} --acts "uboot->enter_uboot"
}

function RecipeSetupNor() {
  ${RUN} --acts "uboot->setup_nor"
}

function RecipeBootOverUart() {
  ${RUN} --acts "uboot->kernel_uart"
}

function RecipeBootOverEth() {
  ${RUN} --acts "uboot->kernel_eth"
}

function RecipeBootOverEthAndDebugKernel() {
  ${RUN} --acts "uboot->kernel_dbg"
}

function RecipeBootOverEthAndWithOldSys() {
  ${RUN} --acts "uboot->kernel_old_sys"
}

function RecipeBootWithSpecTools() {
  ${RUN} --acts "uboot->kernel_eth->mtd_utils"
}

function RecipeBootAndInstallKernel() {
  ${RUN} --acts "uboot->kernel_eth->mtd_utils->install_uboot->install_kernel"
}

function RecipeBootAndUploadRootFS() {
  ${RUN} --acts "uboot->kernel_eth->rootfs"
}

function RecipeBootAndWriteRootFS() {
  ${RUN} --acts "uboot->kernel_eth->mtd_utils->rootfs->unpack_rootfs"
}

function RecipeBootAndInstallRegigraf() {
  ${RUN} --acts "uboot->kernel_eth->install_regigraf"
}

function RecipeSetupBoardForRegigraf() {
  ${RUN} --acts "uboot->setup_nor->kernel_eth->mtd_utils->rootfs->unpack_rootfs->install_regigraf->install_firmware->install_uboot->install_kernel->setup_booting->register"
}

function RecipeTestRegiboard() {
  ${RUN} --acts "uboot->kernel_eth->validate_hw->test_hw"
}

function RecipeDefault() {
  RecipeBootOverEth
}

UpdateFirmwareImage() {
  local rbf=${IMG_RBF_PATH}/${IMG_RBF}
  local mtime1=$(date +'%D')
  local mtime2=$(date --date=@$(stat --printf=%Y ./tmp/cortex_a8.regigraf.1772.53.UNIVERSAL-last.rbf) +'%D')

  if [ "$mtime1" != "$mtime2" ]; then
    echo "Загрузка нового файла прошивки: "
    rm ${IMG_RBF_PATH}/${IMG_RBF}
    wget -P ${IMG_RBF_PATH} ${IMG_RBF_URL}
  fi
}

#echo "Host is: ${HOST_IP}"
UpdateFirmwareImage
RECIPE_FULL_NAME="Recipe${RECIPE}"
${RECIPE_FULL_NAME}

