cmake_minimum_required(VERSION 2.6)
project("psu_test")

add_executable(psu_test
	psu_test.cpp
)

INSTALL(TARGETS psu_test
	RUNTIME DESTINATION "./"
)
