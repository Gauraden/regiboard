#### cmake project
cmake_minimum_required(VERSION 2.6)
project("regirfb")

#### compilation
## executable
add_executable(regirfb
    main.cpp
    framebuf.cpp
)

## libraries
target_link_libraries(regirfb
    png
    z
    rt

    boost_system
    boost_thread
    pthread
)

