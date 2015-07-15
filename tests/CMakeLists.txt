include_directories (
        ${CMAKE_INCLUDE_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/external/gmock-1.7.0/
        ${CMAKE_SOURCE_DIR}/external/gtest-1.7.0/
)

file(GLOB TEST_SOURCES
        "test*.cpp"
)
list(APPEND TEST_SOURCES "allTests.cpp")
add_executable (membrain-tests ${TEST_SOURCES})
add_custom_command(TARGET membrain-tests PRE_BUILD COMMAND ./scripts/gengit.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
target_link_libraries (membrain-tests gmock gtest pthread membrain_static)


file(GLOB TEST_SOURCES
        "test*.cpp"
)
list(APPEND TEST_SOURCES "unitTests.cpp")
add_executable (membrain-unittests ${TEST_SOURCES})
add_custom_command(TARGET membrain-unittests PRE_BUILD COMMAND ./scripts/gengit.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
target_link_libraries (membrain-unittests gmock gtest pthread membrain_static)


file(GLOB TEST_SOURCES
        "test*.cpp"
)
list(APPEND TEST_SOURCES "integrationTests.cpp")
add_executable (membrain-integrationtests ${TEST_SOURCES})
add_custom_command(TARGET membrain-integrationtests PRE_BUILD COMMAND ./scripts/gengit.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
target_link_libraries (membrain-integrationtests gmock gtest pthread membrain_static)


file(GLOB TEST_SOURCES
        "performanceTests.cpp"
        "tester.cpp"
        "performanceTestClasses.cpp"
)
add_executable(membrain-performancetests ${TEST_SOURCES})
add_custom_command(TARGET membrain-performancetests PRE_BUILD COMMAND ./scripts/gengit.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
target_link_libraries (membrain-performancetests pthread membrain_static)


file(GLOB TEST_SOURCES
        "performanceTestsWrapper.cpp"
        "tester.cpp"
        "performanceTestClasses.cpp"
)
add_executable(membrain-performancetestswrapper ${TEST_SOURCES})
add_custom_command(TARGET membrain-performancetestswrapper PRE_BUILD COMMAND ./scripts/gengit.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
target_link_libraries (membrain-performancetestswrapper pthread membrain_static)


find_package(MPI)
if(NOT "${MPI_INCLUDE_PATH}" STREQUAL "")
    set(oldccomp ${CMAKE_C_COMPILER})
    set(CMAKE_C_COMPILER mpicc)
    set(oldcxxcomp ${CMAKE_CXX_COMPILER})
    set(CMAKE_CXX_COMPILER mpicxx)

    message(STATUS "Including mpi specific test")
    include_directories(SYSTEM ${MPI_INCLUDE_PATH})
    file(GLOB TEST_SOURCES
            "mpitest.cpp"
    )
    add_executable(membrain-mpitest ${TEST_SOURCES})
    add_custom_command(TARGET membrain-mpitest PRE_BUILD COMMAND ./scripts/gengit.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    target_link_libraries (membrain-mpitest pthread membrain_static ${MPI_C_LIBRARIES})
else()
    message(WARNING "Omitting mpi specific test")
endif()


file(GLOB TEST_SOURCES
        "memeater.cpp"
)
add_executable(membrain-memeater ${TEST_SOURCES})
add_custom_command(TARGET membrain-memeater PRE_BUILD COMMAND ./scripts/gengit.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
