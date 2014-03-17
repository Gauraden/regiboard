#### cmake project
cmake_minimum_required(VERSION 2.6)
project("regiweb")

#### compilation
## executable
add_executable(regiweb
	gtk_test.cpp
)

## libraries
target_link_libraries(gtk_test
    -pthread
    -lwebkitgtk-1.0
#    -lwebkit-1.0
#    -lgtk-directfb-2.0
    -lsoup-2.4
#    -lgdk-directfb-2.0
    -latk-1.0
    -lpangocairo-1.0
    -lgdk_pixbuf-2.0
    -lpango-1.0
    -lcairo
    -lgio-2.0
    -lgobject-2.0
    -lgmodule-2.0
    -lgthread-2.0
    -lrt
    -lglib-2.0
)

#INSTALL(TARGETS gtk_test
#  RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/output"
#)
