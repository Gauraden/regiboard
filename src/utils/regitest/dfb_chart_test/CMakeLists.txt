cmake_minimum_required(VERSION 2.6)
project("dfb_chart_test")

add_executable(dfb_chart_test
	dfb_chart_test.cpp
	dfb_cairo.cpp
)

target_link_libraries(dfb_chart_test
	++dfb
	directfb
	direct
	fusion
	cairo
	rt
)

INSTALL(TARGETS dfb_chart_test
	RUNTIME DESTINATION "./"
)