file (GLOB SOURCES
    "*.cpp"
)

add_library (membrain SHARED ${SOURCES})
add_library (membrain_static STATIC ${SOURCES})

set_target_properties(membrain_static PROPERTIES OUTPUT_NAME membrain)

if(LOGSTATS)
    target_link_libraries(membrain -lrt)
    target_link_libraries(membrain_static -lrt)
endif()
target_link_libraries(membrain ${AIO_LIBRARIES})
target_link_libraries(membrain_static ${AIO_LIBRARIES})


get_directory_property(defs COMPILE_DEFINITIONS)
message("Compiler definitions: ${defs}")
add_custom_command(TARGET membrain PRE_BUILD COMMAND ../scripts/create-definitions-header.sh ARGS \"${defs}\" WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH})
add_custom_command(TARGET membrain PRE_BUILD COMMAND ./scripts/gengit.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

install (TARGETS membrain DESTINATION lib)
