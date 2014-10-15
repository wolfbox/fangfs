cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project("FangFS")
enable_testing()

add_definitions(-D_FORTIFY_SOURCE=2)
set(CMAKE_C_FLAGS "-Wall -Wextra -Wshadow -Wno-unused-parameter -Werror -g -fstack-protector -O -std=c++0x")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow -Wno-unused-parameter -Werror -g -fstack-protector -O -std=gnu++0x")

add_executable(fangfs src/main.cpp src/fangfs.cpp src/metafile.cpp src/file.cpp src/exlockfile.cpp src/util.cpp)
target_link_libraries(fangfs fuse sodium m)

add_executable(test_path_join tests/path_join.cpp src/util.cpp)
add_test(path_join_test test_path_join)

add_executable(test_endian tests/endian.cpp)
add_test(endian_test test_endian)

add_executable(test_metafile tests/metafile.cpp src/metafile.cpp src/exlockfile.cpp src/util.cpp)
target_link_libraries(test_metafile sodium m)
add_test(metafile_test test_metafile)

add_executable(test_exlockfile tests/exlockfile.cpp src/exlockfile.cpp)
add_test(exlockfile_test test_exlockfile)

add_executable(test_base32 tests/base32.cpp src/util.cpp)
add_test(base32_test test_base32)
