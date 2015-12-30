cmake_minimum_required(VERSION 2.6)
project("udev_test")

add_executable(udev_test
	udev_test.c
)

target_link_libraries(udev_test
  udev
)

INSTALL(TARGETS udev_test
	RUNTIME DESTINATION "./"
)
