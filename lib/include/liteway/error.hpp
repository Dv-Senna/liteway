#pragma once

#include <expected>
#include <format>
#include <source_location>
#include <stack>
#include <string>
#include <tuple>


namespace lw {
	struct ErrorFrame {
		std::string message;
		std::source_location sourceLocation;
	};

	class ErrorStack final {
		ErrorStack(const ErrorStack&) = delete;
		auto operator=(const ErrorStack&) = delete;

		public:
			inline ErrorStack() noexcept = default;
			inline ErrorStack(ErrorStack&&) noexcept = default;
			inline auto operator=(ErrorStack&&) noexcept -> ErrorStack& = default;

			inline auto push(lw::ErrorFrame&& frame) noexcept -> void {
				m_frames.push(std::move(frame));
			}
			inline auto top() const noexcept -> const lw::ErrorFrame& {return m_frames.top();}
			inline auto pop() noexcept -> void {m_frames.pop();}
			inline auto isEmpty() const noexcept -> bool {return m_frames.empty();}

		private:
			std::stack<lw::ErrorFrame> m_frames;
	};

	template <typename T>
	using Failable = std::expected<T, lw::ErrorStack>;


	template <typename ...Args>
	class makeErrorStack {
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
	class pushToErrorStack {
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
			lw::ErrorStack& m_stack;
			lw::ErrorFrame m_frame;
	};

	template <typename T, typename ...Args>
	pushToErrorStack(lw::Failable<T>&, std::format_string<Args...>, Args&&...) -> pushToErrorStack<Args...>;
}
