#pragma once

#include <type_traits>
#include <utility>


namespace lw {
	template <typename Callback>
	class Janitor final {
		public:
			Janitor(const Janitor<Callback>&) = delete;
			auto operator=(const Janitor<Callback>&) = delete;
			Janitor(Janitor<Callback>&&) = delete;
			auto operator=(Janitor<Callback>&&) = delete;

			template <typename Callback2>
			requires (std::is_nothrow_invocable_v<Callback2>)
			constexpr Janitor(Callback2&& callback) noexcept :
				m_callback {std::forward<Callback2> (callback)}
			{}

			template <typename Callback2>
			requires (!std::is_nothrow_invocable_v<Callback2>)
			[[deprecated("Using a callback that can throw is not recommended as it will be called inside"
				" a destructor")]]
			constexpr Janitor(Callback2&& callback) noexcept :
				m_callback {std::forward<Callback2> (callback)}
			{}

			constexpr ~Janitor() {
				m_callback();
			}

		private:
			Callback m_callback;
	};

	template <typename Callback>
	Janitor(Callback&&) -> Janitor<Callback>;
}
