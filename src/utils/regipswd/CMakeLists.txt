cmake_minimum_required(VERSION 2.6)
project("regipswd")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

include_directories("${INC_UBOOT}")

add_definitions(-DUSE_HOSTCC)
# --- uboot --------------------------------------------------------------------
add_library(uboot_generic STATIC
  ${LD_UBOOT}/md5.c
)
# --- util ---------------------------------------------------------------------
add_executable(regipswd
	regipswd.cpp
)

target_link_libraries(regipswd
	boost_program_options
	uboot_generic
)

