#### cmake project
cmake_minimum_required(VERSION 2.6)
project(regitest)

set (CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/.out/")

include(${RB_CROSS_CONF})

add_subdirectory("dfb_test")
add_subdirectory("dfb_cairo_test")
add_subdirectory("dfb_chart_test")
add_subdirectory("rtc_irq_test")
add_subdirectory("sys_stat_test")
add_subdirectory("ttyusb_test")
add_subdirectory("udev_test")
add_subdirectory("usb_test")
add_subdirectory("psu_test")
add_subdirectory("modbus_test")
add_subdirectory("proc_test")
