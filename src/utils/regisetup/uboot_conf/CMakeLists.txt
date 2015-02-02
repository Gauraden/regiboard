cmake_minimum_required(VERSION 2.6)
project("uboot_conf")
#-print-multi-lib -mfloat-abi=softfp
# ------------------------------------------------------------------------------
#arm-cortex_a8-linux-gnueabi-gcc  -g  -Os   -fno-strict-aliasing -fno-common -ffixed-r8 -msoft-float -D__KERNEL__ -#DTEXT_BASE=0x77800000 -I/home/denis/Projects/regiboard/build/u-boot-2009.08.01-regigraf/include -fno-builtin -ffreestanding -#nostdinc -isystem /home/denis/Projects/regiboard/output/toolchain/toolchain.cortex_a8/lib/gcc/arm-cortex_a8-linux-gnueabi/4.5.4/#include -pipe  -DCONFIG_ARM -D__ARM__ -marm  -mabi=aapcs-linux -mno-thumb-interwork -march=armv5   -Wall -Wstrict-prototypes -fno-#stack-protector

#arm-cortex_a8-linux-gnueabi-gcc  -g  -Os   -fno-strict-aliasing -fno-common -ffixed-r8 -msoft-float -D__KERNEL__ -#DTEXT_BASE=0x77800000 -I/home/denis/Projects/regiboard/build/u-boot-2009.08.01-regigraf/include -fno-builtin -ffreestanding -#nostdinc -isystem /home/denis/Projects/regiboard/output/toolchain/toolchain.cortex_a8/lib/gcc/arm-cortex_a8-linux-gnueabi/4.5.4/#include -pipe  -DCONFIG_ARM -D__ARM__ -marm  -mabi=aapcs-linux -mno-thumb-interwork -march=armv5   -Wall -Wstrict-prototypes -fno-#stack-protector   -o crc32.o crc32.c -c

#-Wall -O2 -fomit-frame-pointer -idirafter
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_definitions(
#  -print-multi-lib
#  -Os
#  -fno-strict-aliasing
#  -fno-common
#  -ffixed-r8
#  -msoft-float
#  -mfloat-abi=softfp
#  -marm
#  -mabi=aapcs-linux
#  -mno-thumb-interwork
#  -march=armv5
)

#link_directories("-L${LD_UBOOT}")
include_directories("${INC_UBOOT}")
#add_definitions(-D__KERNEL__)
#add_definitions(-DCONFIG_ARM)
#add_definitions(-D__ARM__)
add_definitions(-DUSE_HOSTCC)
# --- uboot --------------------------------------------------------------------
#set(uboot_object_files
#  ${LD_UBOOT}/crc32.o
#)

add_library(uboot_generic STATIC # EXCLUDE_FROM_ALL
  ${LD_UBOOT}/crc32.c
#  ${uboot_object_files}
)

#set_source_files_properties(
#  ${uboot_object_files}
#  PROPERTIES
#  EXTERNAL_OBJECT true # to say that "this is actually an object file, so it should not be compiled, only linked"
#  GENERATED true       # to say that "it is OK that the obj-files do not exist before build time"
#)

#set_target_properties(uboot_generic
#  PROPERTIES
#  LINKER_LANGUAGE C
#  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib
#)
# --- util ---------------------------------------------------------------------
add_executable(uboot_conf
	uboot_conf.cpp
)

target_link_libraries(uboot_conf
	boost_program_options
	uboot_generic
)

