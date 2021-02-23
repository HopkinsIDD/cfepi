cmake_minimum_required(VERSION 3.6)
project(COVID19_DataAugmentation VERSION 0.0.1 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE DEBUG)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Premade libraries
find_package(stxxl REQUIRED)
## STXXL STUFF
# apply CXXFLAGS to our configuration
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STXXL_CXX_FLAGS}")
# add STXXL include directory
include_directories(${STXXL_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "-Wall -Wextra -latomic")


# Debug flags
if (NOT APPLE)
  set(CMAKE_CXX_FLAGS_DEBUG "-g")
endif()
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg -fsanitize=address -fsanitize=undefined")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

# Release flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(sir_simulation
	src/sir.cpp
)

add_executable(simulation
	src/main.cpp
)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake) # Include Conan-generated file
conan_basic_setup(TARGETS) # Introduce Conan-generated targets

target_include_directories(simulation
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(sir_simulation
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sir_simulation PRIVATE CONAN_PKG::range-v3 ${STXXL_LIBRARIES} )
target_link_libraries(simulation PRIVATE CONAN_PKG::range-v3 ${STXXL_LIBRARIES})
