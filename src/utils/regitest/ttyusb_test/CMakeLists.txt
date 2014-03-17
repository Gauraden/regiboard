cmake_minimum_required(VERSION 2.6)
project("ttyusb_test")

add_executable(ttyusb_test
	ttyusb_test.cpp
)

target_link_libraries(ttyusb_test
	ftdipp
	usb
)

INSTALL(TARGETS ttyusb_test
	RUNTIME DESTINATION "./"
)
