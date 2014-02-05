#### cmake project
cmake_minimum_required(VERSION 2.6)
project("dfb_test")

#### compilation
## executable
add_executable(dfb_test
    dfb_test.cpp
)

## libraries
target_link_libraries(dfb_test
    ++dfb
    directfb
    direct
    fusion
)

