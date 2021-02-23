cmake_minimum_required(VERSION 3.6)
project(COVID19_DataAugmentation VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(${CPPCORO_INCLUDE_DIRS})


set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(a.out
	test_ranges.cpp
)
add_executable(b.out
	tmp.cpp
)

add_executable(c.out
	test_short.cpp
)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake) # Include Conan-generated file
conan_basic_setup(TARGETS) # Introduce Conan-generated targets

target_link_libraries(a.out PRIVATE CONAN_PKG::range-v3)
target_link_libraries(b.out PRIVATE CONAN_PKG::range-v3)
target_link_libraries(c.out PRIVATE CONAN_PKG::range-v3)
