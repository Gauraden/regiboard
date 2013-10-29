#### cmake project
cmake_minimum_required(VERSION 2.6)
project("fcb_gen")

#### compilation
## executable
add_executable(fcb_gen
	fcb_gen.cpp
)

## libraries

#INSTALL(TARGETS ttyusb_test
#	RUNTIME DESTINATION ${OUTPUT_DIR}
#)
