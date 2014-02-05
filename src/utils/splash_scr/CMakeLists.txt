#### cmake project
cmake_minimum_required(VERSION 2.6)
project("splash_src")

set(CMAKE_CXX_FLAGS "-static -s")

#### compilation
## executable
add_executable(splash_scr
	main.cpp
)
