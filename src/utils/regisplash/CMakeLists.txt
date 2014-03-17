#### cmake project
cmake_minimum_required(VERSION 2.6)
project("regisplash")

set(CMAKE_CXX_FLAGS "-static -s")

#### compilation
## executable
add_executable(regisplash
	main.cpp
)
