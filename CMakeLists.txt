cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project("FangFS")
enable_testing()

add_definitions(-D_FORTIFY_SOURCE=2)
set(CMAKE_C_FLAGS "-Wall -Wextra -Wshadow -Wno-unused-parameter -Werror -g -fstack-protector -O -std=gnu99")

add_executable(fangfs src/main.c src/fangfs.c src/metafile.c src/file.c src/exlockfile.c src/util.c)
target_link_libraries(fangfs fuse sodium m)

add_executable(test_path_join tests/path_join.c src/util.c)
add_test(path_join_test test_path_join)

add_executable(test_endian tests/endian.c)
add_test(endian_test test_endian)

add_executable(test_metafile tests/metafile.c src/metafile.c src/exlockfile.c src/util.c)
target_link_libraries(test_metafile sodium m)
add_test(metafile_test test_metafile)

add_executable(test_exlockfile tests/exlockfile.c src/exlockfile.c)
target_link_libraries(test_exlockfile)
add_test(exlockfile_test test_exlockfile)
