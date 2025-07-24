cmake_minimum_required(VERSION 3.20)

project(xdg-shell
	VERSION 1.0.0
	LANGUAGES C
)


set(HEADER_FILE ${CMAKE_CURRENT_BINARY_DIR}/include/xdg-shell/xdg-shell-client-protocol.h)
set(SOURCE_FILE ${CMAKE_CURRENT_BINARY_DIR}/src/xdg-shell-protocol.c)
set(CONFIG_FILE /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml)

make_directory(${CMAKE_CURRENT_BINARY_DIR}/include/xdg-shell)
make_directory(${CMAKE_CURRENT_BINARY_DIR}/src)

add_custom_command(
	OUTPUT
		${HEADER_FILE}
	COMMAND
		wayland-scanner client-header ${CONFIG_FILE} ${HEADER_FILE}
)

add_custom_command(
	OUTPUT
		${SOURCE_FILE}
	COMMAND
		wayland-scanner private-code ${CONFIG_FILE} ${SOURCE_FILE}
)

add_custom_target(xdg-shell-generator DEPENDS ${HEADER_FILE} ${SOURCE_FILE})

add_library(xdg-shell STATIC ${SOURCE_FILE})
target_include_directories(xdg-shell PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include)
add_dependencies(xdg-shell xdg-shell-generator)
add_library(xdg-shell::xdg-shell ALIAS xdg-shell)
