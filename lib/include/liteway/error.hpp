#pragma once

#include <concepts>
#include <expected>
#include <format>
#include <ostream>
#include <print>
#include <queue>
#include <source_location>
#include <string>


namespace lw {
	struct ErrorFrame {
		std::string message;
		std::source_location sourceLocation;
	};

	class ErrorStack final {
		public:
			ErrorStack(const ErrorStack&) = delete;
			auto operator=(const ErrorStack&) = delete;

			inline ErrorStack() noexcept = default;
			inline ~ErrorStack() = default;
			inline ErrorStack(ErrorStack&&) noexcept = default;
			inline auto operator=(ErrorStack&&) noexcept -> ErrorStack& = default;

			inline auto push(lw::ErrorFrame&& frame) noexcept -> void {
				m_frames.push(std::move(frame));
			}
			[[nodiscard]]
			inline auto front() const noexcept -> const lw::ErrorFrame& {return m_frames.front();}
			inline auto pop() noexcept -> void {m_frames.pop();}
			[[nodiscard]]
			inline auto isEmpty() const noexcept -> bool {return m_frames.empty();}

			template <typename T, typename CleanT = std::remove_cvref_t<T>>
			requires std::same_as<CleanT, FILE*> || std::derived_from<CleanT, std::ostream>
			inline auto print(T& file) noexcept -> void {
				if (m_frames.empty())
					return;
				std::println(file, "Error stack:");
				for (; !m_frames.empty(); m_frames.pop()) {
					auto& frame {m_frames.front()};
					std::println(file, "\t- in {} ({}:{}) > {}",
						frame.sourceLocation.function_name(),
						frame.sourceLocation.file_name(),
						frame.sourceLocation.line(),
						frame.message
					);
				}
			}


		private:
			std::queue<lw::ErrorFrame> m_frames;
	};

	template <typename T>
	struct [[nodiscard]] Failable : std::expected<T, lw::ErrorStack> {
		// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
		using std::expected<T, lw::ErrorStack>::expected;
	};


	template <typename ...Args>
	class makeErrorStack final {
		public:
			inline makeErrorStack(
				std::format_string<Args...> format,
				Args&&... args,
				std::source_location location = std::source_location::current()
			) noexcept :
				m_frame {
					.message = std::format(format, std::forward<Args> (args)...),
					.sourceLocation = location
				}
			{}

			template <typename T>
			inline operator lw::Failable<T> () && noexcept {
				ErrorStack errorStack {};
				errorStack.push(std::move(m_frame));
				return std::unexpected{std::move(errorStack)};
			}

		private:
			lw::ErrorFrame m_frame;
	};

	template <typename ...Args>
	makeErrorStack(std::format_string<Args...>, Args&&...) -> makeErrorStack<Args...>;


	template <typename ...Args>
	class pushToErrorStack final {
		public:
			template <typename T>
			inline pushToErrorStack(
				lw::Failable<T>& stack,
				std::format_string<Args...> format,
				Args&&... args,
				std::source_location location = std::source_location::current()
			) noexcept :
				m_stack {stack.error()},
				m_frame {
					.message = std::format(format, std::forward<Args> (args)...),
					.sourceLocation = location
				}
			{}

			template <typename T>
			inline operator lw::Failable<T> () && noexcept {
				m_stack.push(std::move(m_frame));
				return std::unexpected{std::move(m_stack)};
			}

		private:
			// NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
			lw::ErrorStack& m_stack;
			lw::ErrorFrame m_frame;
	};

	template <typename T, typename ...Args>
	pushToErrorStack(lw::Failable<T>&, std::format_string<Args...>, Args&&...) -> pushToErrorStack<Args...>;
}
