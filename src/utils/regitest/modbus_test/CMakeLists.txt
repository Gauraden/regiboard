cmake_minimum_required(VERSION 2.6)
project("modbus_test")

add_executable(modbus_test
	modbus_test.cpp
)

target_link_libraries(modbus_test
	modbus
)

INSTALL(TARGETS modbus_test
	RUNTIME DESTINATION "./"
)
