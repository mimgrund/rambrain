cmake_minimum_required (VERSION 2.8)

project (membrain)

option (BUILD_TESTS "Build tests" ON)
option (SWAPSTATS "Swap stats" ON)

if(SWAPSTATS)
    add_definitions(-DSWAPSTATS)
endif()

find_package (Threads REQUIRED)
if (NOT CMAKE_USE_PTHREADS_INIT)
        error ("Could NOT find pthreads.")
endif ()

set (EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set (LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

set (CMAKE_BUILD_TYPE DEBUG)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -fopenmp -O2 -Wall") # -fPIC -fopenmp -g
#set (CMAKE_EXE_LINKER_FLAGS -Wl  )
if (CMAKE_CXX_COMPILER_ID MATCHES Clang)
	add_definitions (-fcolor-diagnostics -fansi-escape-codes)
endif()

execute_process(COMMAND git log -1 COMMAND head -1 COMMAND cut -d " " -f2 OUTPUT_VARIABLE GIT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
message("Git version is ${GIT_VERSION}")
add_definitions(-D__GIT_VERSION="${GIT_VERSION}")

add_subdirectory (src)
add_subdirectory (external)

if (BUILD_TESTS)
	add_subdirectory (tests)
endif ()

add_custom_target(doc doxygen ${CMAKE_SOURCE_DIR}/doc/doxyfile ${CMAKE_SOURCE_DIR} COMMENT "Generating API documentation with Doxygen" VERBATIM)
