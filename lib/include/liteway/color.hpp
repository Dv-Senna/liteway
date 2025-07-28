#pragma once

#include <cstdint>


namespace lw {
	template <typename T>
	struct BasicColor {
		T r;
		T g;
		T b;
		T a;
	};

	using Color = BasicColor<std::uint8_t>;
	using Colorf = BasicColor<float>;

	constexpr auto colorToUint32(const Color& color) noexcept -> std::uint32_t {
		// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
		return (static_cast<std::uint32_t> (color.a) << 3*8)
			| (static_cast<std::uint32_t> (color.r) << 2*8)
			| (static_cast<std::uint32_t> (color.g) << 8)
			| static_cast<std::uint32_t> (color.b);
		// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
	}
}
