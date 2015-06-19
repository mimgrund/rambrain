file (GLOB SOURCES
    "*.cpp"
)

add_library (membrain ${SOURCES})

get_directory_property(defs COMPILE_DEFINITIONS)
message("Compiler definitions: ${defs}")
add_custom_command(TARGET membrain PRE_BUILD COMMAND ../scripts/create-definitions-header.sh ARGS \"${defs}\" WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH})
add_custom_command(TARGET membrain PRE_BUILD COMMAND ./scripts/gengit.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

install (TARGETS membrain DESTINATION lib)
