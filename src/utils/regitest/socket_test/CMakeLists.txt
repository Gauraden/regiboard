#### cmake project
cmake_minimum_required(VERSION 2.6)
project("temp-2")

set(CMAKE_CXX_FLAGS "")

#### configuration
## includes

include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")

link_directories("${TOOLCHAIN_STAGE_DIR}/usr/lib")

#### compilation
## executable
add_executable(simple_server
	simple_server.cpp
)

add_executable(simple_client
	simple_client.cpp
)


## libraries
target_link_libraries(simple_server
	# ---- our sources
#	boost_program_options
#	hardware
)

#### cpack
## install: helpers to generate build
install_targets(/ simple_server)
install_targets(/ simple_client)
