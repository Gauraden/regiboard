cmake_minimum_required(VERSION 2.6)
project("rtc_irq_test")

add_executable(rtc_irq_test
	rtc_irq_test.cpp
)

INSTALL(TARGETS rtc_irq_test
	RUNTIME DESTINATION "./"
)
