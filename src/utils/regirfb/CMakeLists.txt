#### cmake project
cmake_minimum_required(VERSION 2.6)
project("regirfb")

add_definitions(
  --std=gnu++0x
)

#### compilation
## executable
add_executable(regirfb
    main.cpp
    framebuf.cpp
    ipc.cpp
)

## libraries
target_link_libraries(regirfb
    pthread
    z
    rt
    boost_thread
    boost_system
    boost_filesystem
    boost_date_time
    boost_regex
    boost_program_options
    webapp_lib
)

