#### cmake project
cmake_minimum_required(VERSION 2.6)
project(regisetup)

set (CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/.out/")

add_subdirectory("uboot_conf")
