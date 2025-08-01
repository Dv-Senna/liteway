set(SUBPROJECT_DEPENDENCIES "xdg-shell")
if (PROJECT_IS_TOP_LEVEL)
	set(VENDORS_DIR ${PROJECT_SOURCE_DIR}/vendors)
else()
	set(VENDORS_DIR ${CMAKE_CURRENT_LIST_DIR}/../vendors)
endif()


macro(find_package package)
	if (NOT "${package}" IN_LIST SUBPROJECT_DEPENDENCIES)
		_find_package(${ARGV})
	else()
		add_subdirectory(${VENDORS_DIR}/${package})
	endif()
endmacro()
