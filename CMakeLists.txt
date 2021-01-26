cmake_minimum_required (VERSION 2.8)

project (PANGTAO)

add_subdirectory(src)

add_executable(bin/test_log tests/test_log.cpp)
add_executable(bin/test_configure tests/test_configure.cpp)
target_link_libraries(bin/test_log SRC)
target_link_libraries(bin/test_configure SRC)