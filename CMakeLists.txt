cmake_minimum_required (VERSION 2.8)

project (membrain)

option (BUILD_TESTS "Build tests" ON)
option (PTEST_CHECKS "Include correctness checks in performance tests" OFF)
option (SWAPSTATS "Output swap stats on signal SIGUSR1" ON)
option (LOGSTATS "Log stats instead of outputting them to stdout" OFF)
option (PARENTAL_CONTROL "Hook up chunks in a tree" OFF)
option (CAN_IGNORE_WARNINGS "Enable warning ignore macros to be used to suppress unnescessary stuff" ON)

if(SWAPSTATS)
    add_definitions(-DSWAPSTATS)
endif()

if(LOGSTATS)
    if(SWAPSTATS)
        add_definitions(-DLOGSTATS)
    else(SWAPSTATS)
        message(FATAL_ERROR "If you want to log stats you also have to enable SWAPSTATS. Exiting cmake.")
    endif(SWAPSTATS)
endif()

if(PARENTAL_CONTROL)
    add_definitions(-DPARENTAL_CONTROL)
endif()

if(CAN_IGNORE_WARNINGS)
    add_definitions(-DCAN_IGNORE_WARNINGS)
endif()


find_package (Threads REQUIRED)
if (NOT CMAKE_USE_PTHREADS_INIT)
    error ("Could NOT find pthreads.")
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})
set (EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set (LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

find_package(AIO REQUIRED)
include_directories(${AIO_INCLUDE_DIR})

set (CMAKE_BUILD_TYPE DEBUG)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -fopenmp -O0 -Wall -g")
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

execute_process(COMMAND ./scripts/gengit.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
