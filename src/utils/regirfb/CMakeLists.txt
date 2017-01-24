#### cmake project
cmake_minimum_required(VERSION 2.6)
project("regirfb")

#### compilation
## executable
add_executable(regirfb
    main.cpp
    framebuf.cpp
    ipc.cpp
)

## libraries
target_link_libraries(regirfb
    z
    rt
    pthread
    boost_system
    boost_filesystem
    boost_date_time
    boost_program_options
#    boost_thread
    webapp_lib
)

