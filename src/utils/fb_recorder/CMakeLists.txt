#### cmake project
cmake_minimum_required(VERSION 2.6)
project("fb_rd")

set(CMAKE_CXX_FLAGS "-static -s")

include_directories("${PROJECT_SOURCE_DIR}")

#### compilation
## executable
add_executable(fb_rd
    main.cpp
    framebuf.cpp
)

## libraries
target_link_libraries(fb_rd
    png
    z
    rt
)

INSTALL(TARGETS fb_rd
  RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}"
)
