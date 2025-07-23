#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>


namespace lw {
	template <typename T>
	requires (std::is_pointer_v<T>)
	class Owned {
		Owned(const Owned<T>&) = delete;
		auto operator=(const Owned<T>&) = delete;

		using ElementType = typename std::pointer_traits<T>::element_type;

		public:
			constexpr Owned() noexcept : m_ptr {nullptr} {}
			explicit constexpr Owned(std::nullptr_t) noexcept : m_ptr {nullptr} {}
			constexpr auto operator=(std::nullptr_t) noexcept -> Owned<T>& {
				m_ptr = nullptr;
				return *this;
			}
			explicit constexpr Owned(T ptr) noexcept : m_ptr {ptr} {}
			constexpr Owned(Owned<T>&& other) noexcept : m_ptr {other.m_ptr} {
				other.m_ptr = nullptr;
			}
			constexpr auto operator=(Owned<T>&& other) noexcept -> Owned<T>& {
				m_ptr = other.m_ptr;
				other.m_ptr = nullptr;
				return *this;
			}

			constexpr auto operator==(std::nullptr_t) const noexcept -> bool {
				return m_ptr == nullptr;
			}

			explicit constexpr operator bool() const noexcept {
				return !!m_ptr;
			}
			constexpr operator T() const noexcept {
				return this->get();
			}

			constexpr auto operator*() const noexcept -> ElementType& {return *m_ptr;}
			constexpr auto operator->() const noexcept -> T {return m_ptr;}

			constexpr auto get() const noexcept -> T {return m_ptr;}
			constexpr auto release() noexcept -> T {
				T ptr {m_ptr};
				m_ptr = nullptr;
				return ptr;
			};

		private:
			T m_ptr;
	};

	template <typename T>
	constexpr auto operator==(std::nullptr_t, const Owned<T>& owned) noexcept -> bool {
		return owned == nullptr;
	}
}
