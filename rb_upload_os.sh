#!/bin/bash

. ./.config

RECIPE=${1:-"Help"}
TTY_USB=${2:-/dev/ttyUSB0}
REGILOADER="bin/regiloader"
IMG_UBOOT="output/uboot/u-boot.regigraf.imx53"
IMG_KERNEL="output/kernel/uImage.regigraf.imx53.bin"
IMG_KERNEL_MODS="tmp/linux-kernel-modules.ipk/data.tar.gz"
IMG_ROOTFS="output/rootfs/rootfs.regigraf.cortex_a8.tar.gz"
IMG_MTD_UTILS="tmp/mtd-utils.ipk/data.tar.gz"
IMG_RBF="cortex_a8.regigraf.1772.53.UNIVERSAL-last.rbf"
IMG_RBF_PATH="tmp/"
IMG_RBF_URL="ftp://jenny/firmwares/F1772/${IMG_RBF}"

HOST_IP=$(ifconfig | grep -oE "(inet |inet addr:)192.([0-9\.]+)(.+)(netmask |Mask:)([0-9\.]+)")
HOST_MASK=$(echo $HOST_IP | grep -oE "(netmask |Mask:)([0-9\.]+)" | grep -oE "([0-9\.]+)")
HOST_IP=$(echo $HOST_IP | grep -oE "(inet |inet addr:)([0-9\.]+)" | grep -oE "([0-9\.]+)")

RUN="$REGILOADER --tty ${TTY_USB} --img ${IMG_UBOOT} --kernel ${IMG_KERNEL}
--kernel_mods ${IMG_KERNEL_MODS}
--tftp ${HOST_IP}/${HOST_MASK} --rootfs ${IMG_ROOTFS} --utils ${IMG_MTD_UTILS} --conf ./tmp
--rbf ${IMG_RBF_PATH}${IMG_RBF} --uboot_pswd ${UBOOT_PSWD} ${RBUP_EXT_OPTS}"

function RecipeBootloader() { # загрузка, запуск и вход в UBoot
  ${RUN} --acts "uboot->enter_uboot"
}

function RecipeSetupNor() { # загрузка UBoot и подготовка NOR
  ${RUN} --acts "uboot->setup_nor"
}

function RecipeBootOverUart() { # загрузка ядра Linux через UART
  ${RUN} --acts "uboot->kernel_uart"
}

function RecipeBootOverEth() { # загрузка ядра Linux через Ethernet
  ${RUN} --acts "uboot->kernel_eth"
}

function RecipeBootOverEthAndDebugKernel() { # загрузка ядра Linux в отладочном режиме
  ${RUN} --acts "uboot->kernel_dbg"
}

function RecipeBootOverEthAndWithOldSys() { # загрузка нового ядра Linux в старом окружении
  ${RUN} --acts "uboot->kernel_old_sys"
}

function RecipeBootWithSpecTools() { # загрузка ядра Linux и установка служебных утилит
  ${RUN} --acts "uboot->kernel_eth->mtd_utils"
}

function RecipeBootAndInstallKernel() { # прошивка загрузчика UBoot и ядра Linux
  ${RUN} --acts "uboot->kernel_eth->mtd_utils->install_uboot->install_kernel"
}

function RecipeBootAndUploadRootFS() { # загрузка ядра Linux и образа rootfs
  ${RUN} --acts "uboot->kernel_eth->rootfs"
}

function RecipeBootAndWriteRootFS() { # загрузка ядра Linux и прошивка rootfs
  ${RUN} --acts "uboot->kernel_eth->mtd_utils->rootfs->unpack_rootfs"
}

function RecipeBootAndInstallRegigraf() { # загрузка ядра Linux и установка ПО Regigraf
  ${RUN} --acts "uboot->kernel_eth->install_regigraf"
}
# --- Основные рецепты ---------------------------------------------------------
function RecipeUpdateRootFS() { # обновление rootfs и переустановка всех программ
  ${RUN} --acts "uboot->kernel_eth->mtd_utils->rootfs->unpack_rootfs->install_regigraf->install_firmware->register"
}

function RecipeSetupBoardForRegigraf() { # загрузка, прошивка и установка всего необходимого ПО для Regigraf
  ${RUN} --acts "uboot->setup_nor->kernel_eth->mtd_utils->rootfs->unpack_rootfs->install_regigraf->install_firmware->install_uboot->install_kernel->setup_booting->register"
}

function RecipeSetupBoardForRegigrafWithHWtest() { # загрузка, прошивка и установка всего необходимого ПО для Regigraf, без проверки конфигурации периферии
  ${RUN} --acts "uboot->setup_nor->kernel_eth->validate_hw->mtd_utils->rootfs->unpack_rootfs->install_regigraf->install_firmware->install_uboot->install_kernel->setup_booting->register"
}

function RecipeTestRegiboard() { # проверка конфигурации периферии и её тестирование
#  ${RUN} --acts "uboot->kernel_eth->validate_hw->test_hw"
  ${RUN} --acts "uboot->kernel_eth->validate_hw"
}

function RecipeTestTFTP() { # проверка загрузки файлов по протоколу TFTP
  ${RUN} --acts "mtd_utils->rootfs"
}

function RecipeDefault() { # действие по умолчанию, загрузка ядра Linux
  RecipeBootOverEth
}

function RecipeGetListOfSupportedLCD() { # вывод списка поддерживаемых дисплеев
  kernel_src='./build/linux-2.6.35.3.SK/arch/arm/mach-mx5/mx53_regigraf.c'
  echo "LCD:"
  while read line           
  do           
    echo "$line" | grep -oE "\.mode_str(.+)\= \"(.+)" | sed -r "s/(.+)\= \"([^\"]+)\"(.+)/\t * \2/"
  done < $kernel_src 
}

function RecipeHelp() { # вывод помощи
  local BLUE='\033[1;34m'
  local RESET='\033[0m'
  echo "Список функций:"
  local funcs_list=$(grep "^function" $0 | sed -r "s/function Recipe(.+)\(\)(.+)\{ \# (.+)/\t * \\$BLUE\1\\$RESET: \n\t   \3/")
  echo -e "$funcs_list"
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

echo "Хост: ${HOST_IP}"
UpdateFirmwareImage
RECIPE_FULL_NAME="Recipe${RECIPE}"
${RECIPE_FULL_NAME}

