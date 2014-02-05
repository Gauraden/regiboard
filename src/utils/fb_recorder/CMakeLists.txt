#### cmake project
cmake_minimum_required(VERSION 2.6)
project("fb_recorder")

#set(CMAKE_CXX_FLAGS "-static -s ${CMAKE_CXX_FLAGS}")

#### compilation
## executable
add_executable(fb_recorder
    main.cpp
    framebuf.cpp
)

## libraries
target_link_libraries(fb_recorder
    png
    z
    rt
)

#INSTALL(TARGETS fb_rd
#  RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}"
#)
