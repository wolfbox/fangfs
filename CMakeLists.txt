INCLUDE(CheckSymbolExists)
INCLUDE(CheckFunctionExists)
INCLUDE(FindPkgConfig)

cmake_minimum_required(VERSION 2.8)
project("FangFS")
enable_testing()

CHECK_FUNCTION_EXISTS(fdopendir HAVE_FDOPENDIR)
CHECK_SYMBOL_EXISTS(_SC_PHYS_PAGES unistd.h HAVE_SC_PHYS_PAGES)
CHECK_SYMBOL_EXISTS(HW_MEMSIZE sys/sysctl.h HAVE_HW_MEMSIZE)

SET(UTIL_SOURCE src/exlockfile.cpp src/util.cpp src/Buffer.cpp)
if(HAVE_FDOPENDIR)
    add_definitions(-DHAVE_FDOPENDIR)
else()
    LIST(APPEND UTIL_SOURCE src/compat/fdopendir.cpp)
endif()

if(HAVE_SC_PHYS_PAGES)
	add_definitions(-DHAVE_SC_PHYS_PAGES)
elseif(HAVE_HW_MEMSIZE)
	add_definitions(-DHAVE_HW_MEMSIZE)
else()
	message(FATAL_ERROR "Could not find any way of detecting system memory")
endif()

SET(SOURCE src/fangfs.cpp src/metafile.cpp src/file.cpp ${UTIL_SOURCE})

pkg_check_modules(FUSE REQUIRED fuse)
INCLUDE_DIRECTORIES(${FUSE_INCLUDE_DIRS})

add_definitions(-D_FORTIFY_SOURCE=2)
set(CMAKE_C_FLAGS "-Wall -Wextra -Wshadow -Wno-unused-parameter -Werror -g -fstack-protector -O -std=c++0x")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow -Wno-unused-parameter -Werror -g -fstack-protector -O -std=gnu++0x")

add_executable(fangfs src/main.cpp ${SOURCE})
target_link_libraries(fangfs ${FUSE_LIBRARIES} sodium m)

add_executable(test_path_join tests/paths.cpp ${UTIL_SOURCE})
add_test(path_join_test test_path_join)

add_executable(test_endian tests/endian.cpp)
add_test(endian_test test_endian)

add_executable(test_metafile tests/metafile.cpp ${SOURCE})
target_link_libraries(test_metafile sodium m)
add_test(metafile_test test_metafile)

add_executable(test_exlockfile tests/exlockfile.cpp ${UTIL_SOURCE})
add_test(exlockfile_test test_exlockfile)

add_executable(test_base32 tests/base32.cpp ${UTIL_SOURCE})
add_test(base32_test test_base32)
