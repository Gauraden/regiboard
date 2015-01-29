cmake_minimum_required(VERSION 2.6)
project("uboot_conf")

add_executable(uboot_conf
	uboot_conf.cpp
)

target_link_libraries(uboot_conf
	boost_program_options
)

