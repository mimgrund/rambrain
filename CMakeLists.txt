lkcmake_minimum_required (VERSION 2.8)

project (membrain)

option (BUILD_TESTS "Build tests" ON)

set (EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set (LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

set (CMAKE_BUILD_TYPE DEBUG)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -O2 -Wall")
if (CMAKE_CXX_COMPILER_ID MATCHES Clang)
	add_definitions (-fcolor-diagnostics -fansi-escape-codes)
endif()

add_subdirectory (src)
add_subdirectory (external)

if (BUILD_TESTS)
	add_subdirectory (tests)
endif ()
