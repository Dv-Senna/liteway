#pragma once

#include "liteway/utils.hpp"

#ifdef __cpp_impl_reflection
	#include <meta>
#endif


namespace lw::utils {
	template <typename T>
	consteval auto getTypeName() noexcept -> std::string_view {
	#ifdef __cpp_impl_reflection
		return identifier_of(^^T);
	#else
		using namespace std::string_view_literals;
		std::string_view name {std::source_location::current().function_name()};
		#ifdef __clang__
			name = name.substr(name.find("T = ") + "T = "sv.size());
			name = name.substr(0, name.find_first_of(']'));
		#elifdef __GNUC__
			name = name.substr(name.find("T = ") + "T = "sv.size());
			name = name.substr(0, name.find_first_of(';'));
		#elifdef _MSC_VER
			name = name.substr(name.find("getTypeName") + "getTypeName"sv.size());
			const auto structPosition {name.find("struct ")};
			if (structPosition != name.npos)
				name = name.substr(structPosition + "struct "sv.size());
			name = name.substr(0, name.find_last_of('>'));
		#endif
		return name;
	#endif
	}
}
