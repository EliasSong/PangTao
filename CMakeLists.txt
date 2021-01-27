cmake_minimum_required (VERSION 2.8)

project (PANGTAO)
find_library(PTHREAD pthread)
add_subdirectory(src)

add_executable(bin/test_log tests/test_log.cpp)
add_executable(bin/test_configure tests/test_configure.cpp)
add_executable(bin/test_thread tests/test_thread.cpp)

target_link_libraries(bin/test_log SRC ${PTHREAD})
target_link_libraries(bin/test_configure SRC ${PTHREAD})
target_link_libraries(bin/test_thread SRC ${PTHREAD})
