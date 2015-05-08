file (GLOB SOURCES
    "*.cpp"
)

add_library (membrain ${SOURCES})

get_directory_property(defs COMPILE_DEFINITIONS)
message("Compiler definitions: ${defs}")
add_custom_command(TARGET membrain PRE_BUILD COMMAND ../scripts/create-definitions-header.sh ARGS \"${defs}\" WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH})

install (TARGETS membrain DESTINATION lib)
