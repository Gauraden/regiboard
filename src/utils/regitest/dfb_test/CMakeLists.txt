cmake_minimum_required(VERSION 2.6)
project("dfb_test")

add_executable(dfb_test
	dfb_test.cpp
)

target_link_libraries(dfb_test
	++dfb
	directfb
	direct
	fusion
)

INSTALL(TARGETS dfb_test
	RUNTIME DESTINATION "./"
)
