#pragma once

#include <type_traits>
#include <utility>


namespace lw {
	template <typename Callback>
	class Janitor final {
		public:
			constexpr Janitor(Callback&& callback) noexcept requires (std::is_nothrow_invocable_v<Callback>) :
				m_callback {std::forward<Callback> (callback)}
			{}

			[[deprecated("Using a callback that can throw is not recommended as it will be called inside\
				a destructor")]]
			constexpr Janitor(Callback&& callback) noexcept requires (!std::is_nothrow_invocable_v<Callback>) :
				m_callback {std::forward<Callback> (callback)}
			{}

			constexpr ~Janitor() {
				m_callback();
			}

		private:
			Callback m_callback;
	};
}
