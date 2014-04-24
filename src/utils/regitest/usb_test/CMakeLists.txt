cmake_minimum_required(VERSION 2.6)
project("usb_test")

add_executable(usb_test
	usb_test.cpp
)

target_link_libraries(usb_test
	usb-1.0
	usb
)

INSTALL(TARGETS usb_test
	RUNTIME DESTINATION "./"
)