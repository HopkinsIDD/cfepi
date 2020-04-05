cmake_minimum_required(VERSION 3.6)
project(COVID19_DataAugmentation VERSION 0.0.1 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE DEBUG)
endif()

set(CMAKE_CXX_FLAGS "-fconcepts -Wall -Wextra -std=c++17 -latomic")

# Debug flags
if (NOT APPLE)
  set(CMAKE_CXX_FLAGS_DEBUG "-pg")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fsanitize=undefined -g")

# Release flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(sir_simulation
	src/sir.cpp
)

add_executable(simulation
	src/main.cpp
)

target_include_directories(simulation
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(sir_simulation
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)
