cmake_minimum_required(VERSION 2.6)
project("dfb_cairo_test")

add_executable(dfb_cairo_test
	dfb_cairo_test.cpp
)

target_link_libraries(dfb_cairo_test
	++dfb
	directfb
	direct
	fusion
	cairo
)

INSTALL(TARGETS dfb_cairo_test
	RUNTIME DESTINATION "./"
)