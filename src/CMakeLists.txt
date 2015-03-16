file (GLOB SOURCES
    "*.cpp"
)

add_library (membrain ${SOURCES})
install (TARGETS hplotlib DESTINATION lib)
