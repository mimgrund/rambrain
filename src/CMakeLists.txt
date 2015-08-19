file (GLOB SOURCES
    "*.cpp"
)

add_library (rambrain SHARED ${SOURCES})
add_library (rambrain_static STATIC ${SOURCES})

set_target_properties(rambrain_static PROPERTIES OUTPUT_NAME rambrain)

if(LOGSTATS)
    target_link_libraries(rambrain -lrt)
    target_link_libraries(rambrain_static -lrt)
endif()
target_link_libraries(rambrain ${AIO_LIBRARIES})
target_link_libraries(rambrain_static ${AIO_LIBRARIES})


get_directory_property(defs COMPILE_DEFINITIONS)
message("Compiler definitions: ${defs}")
add_custom_command(TARGET rambrain PRE_BUILD COMMAND ../scripts/create-definitions-header.sh ARGS \"${defs}\" WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH})
add_custom_command(TARGET rambrain PRE_BUILD COMMAND ./scripts/gengit.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

install (CODE "MESSAGE(\"Installing shared library and headers (into rambrain subdir)\")")

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)

if ("${LIB64}" STREQUAL "TRUE")
    set(LIBSUFFIX 64)
else()
    set(LIBSUFFIX "")
endif()

install (TARGETS rambrain DESTINATION lib${LIBSUFFIX})
install (DIRECTORY ${CMAKE_SOURCE_DIR}/src/ DESTINATION include/rambrain FILES_MATCHING PATTERN "*.h")
install (FILES ${LIBRARY_OUTPUT_PATH}/rambrainDefinitionsHeader.h DESTINATION include/rambrain)
