#### cmake project
cmake_minimum_required(VERSION 2.6)
project("rtc_irq_test")

#### compilation
## executable
add_executable(rtc_irq_test
	rtc_irq_test.cpp
)

## libraries
#target_link_libraries(rtc_irq_test
#)

#INSTALL(TARGETS  rtc_irq_test
#	RUNTIME DESTINATION ${OUTPUT_DIR}
#)
