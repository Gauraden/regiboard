cmake_minimum_required(VERSION 2.6)
project("dfb_cairo_test")

message(WARNING "source dir: ${PROJECT_SOURCE_DIR}")

set(CMAKE_MODULE_PATH  ${PROJECT_SOURCE_DIR}/cmake-extensions/ )
find_package(PkgTCConfig)
pkg_check_modules(LIBRSVG librsvg-2.0 REQUIRED)

add_executable(dfb_cairo_test
	dfb_cairo_test.cpp
)

if (LIBRSVG_INCLUDE_DIRS)
    include_directories(${LIBRSVG_INCLUDE_DIRS})
endif ()
if (LIBRSVG_LIBRARY_DIRS)
    link_directories(${LIBRSVG_LIBRARY_DIRS})
endif ()


target_link_libraries(dfb_cairo_test
	++dfb
	directfb
	direct
	fusion
	cairo
	gobject-2.0
	glib-2.0
	rsvg-2
	gdk_pixbuf-2.0
)

INSTALL(TARGETS dfb_cairo_test
	RUNTIME DESTINATION "./"
)
