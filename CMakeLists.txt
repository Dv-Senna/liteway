cmake_minimum_required(VERSION 3.25)

project(liteway
	VERSION 0.0.1
	LANGUAGES CXX
)

add_subdirectory(lib EXCLUDE_FROM_ALL)
add_subdirectory(examples EXCLUDE_FROM_ALL)
