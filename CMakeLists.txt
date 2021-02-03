cmake_minimum_required (VERSION 2.8)

project (PANGTAO)
find_library(PTHREAD pthread)
add_subdirectory(src)

add_executable(bin/test_log tests/test_log.cpp)
add_executable(bin/test_configure tests/test_configure.cpp)
add_executable(bin/test_thread tests/test_thread.cpp)
add_executable(bin/test_util tests/test_util.cpp)
add_executable(bin/test_coroutine tests/test_coroutine.cpp)
add_executable(bin/test_scheduler tests/test_scheduler.cpp)

target_link_libraries(bin/test_log SRC ${PTHREAD})
target_link_libraries(bin/test_configure SRC ${PTHREAD})
target_link_libraries(bin/test_thread SRC ${PTHREAD})
target_link_libraries(bin/test_util SRC ${PTHREAD})
target_link_libraries(bin/test_coroutine SRC ${PTHREAD})
target_link_libraries(bin/test_scheduler SRC ${PTHREAD})