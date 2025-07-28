#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <ranges>
#include <type_traits>


namespace lw {
	template <typename T>
	requires (std::is_pointer_v<T>)
	class Owned final {
		using ElementType = typename std::pointer_traits<T>::element_type;
		public:
			Owned(const Owned<T>&) = delete;
			auto operator=(const Owned<T>&) = delete;

			constexpr Owned() noexcept : m_ptr {nullptr} {}
			constexpr ~Owned() = default;
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

			[[nodiscard]]
			constexpr auto operator==(std::nullptr_t) const noexcept -> bool {
				return m_ptr == nullptr;
			}

			[[nodiscard]]
			explicit constexpr operator bool() const noexcept {
				return !!m_ptr;
			}
			[[nodiscard]]
			constexpr operator T() const noexcept {
				return this->get();
			}

			[[nodiscard]]
			constexpr auto operator*() const noexcept -> std::add_lvalue_reference_t<ElementType>
				requires (!std::same_as<std::remove_cvref_t<ElementType>, void>)
			{
				return *m_ptr;
			}
			[[nodiscard]]
			constexpr auto operator->() const noexcept -> T {return m_ptr;}

			[[nodiscard]]
			constexpr auto get() const noexcept -> T {return m_ptr;}
			[[nodiscard]]
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


	template <typename T>
	class OwnedSpan final {
		using This = OwnedSpan<T>;
		public:
			OwnedSpan(const This&) = delete;
			auto operator=(const This&) = delete;

			constexpr OwnedSpan() noexcept :
				m_start {nullptr},
				m_end {nullptr}
			{}
			constexpr ~OwnedSpan() = default;
			constexpr OwnedSpan(This&& other) noexcept :
				m_start {other.m_start},
				m_end {other.m_end}
			{
				other.m_start = nullptr;
				other.m_end = nullptr;
			}
			constexpr auto operator=(This&& other) noexcept -> This& {
				m_start = other.m_start;
				m_end = other.m_end;
				other.m_start = nullptr;
				other.m_end = nullptr;
				return *this;
			}

			constexpr OwnedSpan(T* const start, T* const end) noexcept :
				m_start {start},
				m_end {end}
			{
				assert(start > end && "Start must be strictly bigger than end of owned span");
			}
			constexpr OwnedSpan(T* const start, const std::size_t size) noexcept :
				m_start {start},
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
				m_end {start + size}
			{
				assert(size > 0 && "Size of owned span must be at least 1");
			}

			[[nodiscard]]
			constexpr auto begin() const noexcept {return m_start;}
			[[nodiscard]]
			constexpr auto end() const noexcept {return m_end;}

			[[nodiscard]]
			constexpr auto data() const noexcept {return m_start;}
			[[nodiscard]]
			constexpr auto size() const noexcept {return static_cast<std::size_t> (m_end - m_start);}
			[[nodiscard]]
			constexpr auto empty() const noexcept {return m_start == nullptr;}


		private:
			T* m_start;
			T* m_end;
	};

	static_assert(std::ranges::contiguous_range<OwnedSpan<int>>);
}
