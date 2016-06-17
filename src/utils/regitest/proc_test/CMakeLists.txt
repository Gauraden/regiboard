cmake_minimum_required(VERSION 2.6)
project("proc_test")

add_executable(proc_test
	proc_test.cpp
)

INSTALL(TARGETS proc_test
	RUNTIME DESTINATION "./"
)
