#pragma once

#include <source_location>
#include <string_view>


namespace lw::utils {
	template <typename T>
	consteval auto getTypeName() noexcept -> std::string_view;
}

#include "liteway/utils.inl"
