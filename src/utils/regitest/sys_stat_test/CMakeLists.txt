cmake_minimum_required(VERSION 2.6)
project("sys_stat_test")

add_executable(sys_stat_test
	stat_test.cpp
)

INSTALL(TARGETS sys_stat_test
	RUNTIME DESTINATION "./"
)