cmake_minimum_required(VERSION 2.6)
project("uboot_conf")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

include_directories("${INC_UBOOT}")

add_definitions(-DUSE_HOSTCC)
# --- uboot --------------------------------------------------------------------
add_library(uboot_generic STATIC
  ${LD_UBOOT}/crc32.c
)
# --- util ---------------------------------------------------------------------
add_executable(uboot_conf
	uboot_conf.cpp
)

target_link_libraries(uboot_conf
	boost_program_options
	uboot_generic
)

