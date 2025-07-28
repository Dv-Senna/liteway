include(GenerateExportHeader)

function(generate_export_header TARGET EXPORT_FILENAME)
	set(GENERATE_EXPORT_HEADERS_UNDEF_DEPRECATION "
		#undef LW_DEPRECATED
		#undef LW_DEPRECATED_EXPORT
		#undef LW_DEPRECATED_NO_EXPORT
	")
	_generate_export_header(${TARGET}
		EXPORT_FILE_NAME ${EXPORT_FILENAME}
		BASE_NAME "LW"
		CUSTOM_CONTENT_FROM_VARIABLE GENERATE_EXPORT_HEADERS_UNDEF_DEPRECATION
	)
endfunction()

file(GLOB_RECURSE SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(liteway-interface-common INTERFACE)
target_compile_features(liteway-interface-common INTERFACE cxx_std_23)
target_include_directories(liteway-interface-common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(liteway-interface-common INTERFACE wayland-client xdg-shell::xdg-shell)

add_library(liteway-private-common INTERFACE)
if (MSVC)
	target_compile_options(liteway-private-common INTERFACE /EHsc /W4)
else()
	target_compile_options(liteway-private-common INTERFACE -fno-exceptions -Wall -Wextra -Wpedantic)
endif()


add_library(liteway-static STATIC ${SOURCE_FILE})
add_library(liteway::static ALIAS liteway-static)
target_link_libraries(liteway-static
	PUBLIC
		liteway-interface-common
	PRIVATE
		liteway-private-common
)
target_include_directories(liteway-static PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/generated/static/include)
generate_export_header(liteway-static ${CMAKE_CURRENT_BINARY_DIR}/generated/static/include/liteway/export.hpp)


add_library(liteway-shared SHARED ${SOURCE_FILE})
add_library(liteway::shared ALIAS liteway-shared)
target_link_libraries(liteway-shared
	PUBLIC
		liteway-interface-common
	PRIVATE
		liteway-private-common
)
target_include_directories(liteway-shared PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/generated/shared/include)
generate_export_header(liteway-shared ${CMAKE_CURRENT_BINARY_DIR}/generated/shared/include/liteway/export.hpp)



