cmake_minimum_required(VERSION 2.6)
project("regiloader")

add_definitions(
  --std=gnu++0x
)

set(CMAKE_CXX_FLAGS "")

add_executable(regiloader
	regiloader.cpp
	sdp.cpp
	ymodem.cpp
	tftp.cpp
	debug_backtrace.cpp
	asio_helper.cpp
)

target_link_libraries(regiloader
  pthread
  boost_thread
  boost_system
  boost_filesystem
  boost_program_options
  boost_regex
)

