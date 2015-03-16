include_directories (
	${CMAKE_CURRENT_SOURCE_DIR}
)

add_library (gtest gtest-all.cc)
target_link_libraries (gtest)
