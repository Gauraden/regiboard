#### cmake project
cmake_minimum_required(VERSION 2.6)
project("udev_handler")

#### compilation
## executable
add_executable(udev_handler
	udev_test.c
)

## libraries

#INSTALL(TARGETS udev_handler
#	RUNTIME DESTINATION ${OUTPUT_DIR}
#)
