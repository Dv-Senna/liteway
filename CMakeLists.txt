cmake_minimum_required(VERSION 3.25)

project(liteway
	VERSION 0.0.1
	LANGUAGES CXX
)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/utils.cmake)

find_package(xdg-shell REQUIRED)

add_subdirectory(lib EXCLUDE_FROM_ALL)
add_subdirectory(examples EXCLUDE_FROM_ALL)
