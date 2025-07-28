#pragma once

#include <cstdint>
#include <string_view>

#include <xdg-shell/xdg-shell-client-protocol.h>
#include <wayland-client.h>

#include "liteway/color.hpp"
#include "liteway/error.hpp"
#include "liteway/export.hpp"
#include "liteway/pointer.hpp"


namespace lw::wayland {
	class Instance;

	class LW_EXPORT Window final {
		public:
			Window(const Window&) = delete;
			auto operator=(const Window&) = delete;

			inline Window() noexcept = default;
			inline Window(Window&&) noexcept = default;
			inline auto operator=(Window&&) noexcept -> Window& = default;
			~Window();

			struct CreateInfos {
				// NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
				Instance& instance;
				std::string_view title;
				std::uint32_t width;
				std::uint32_t height;
			};

			static auto create(const CreateInfos& createInfos) noexcept -> lw::Failable<Window>;

			auto fill(const lw::Color& color) noexcept -> lw::Failable<void>;

		private:
			static auto s_createAnonymousFile(std::string_view name, std::size_t size) noexcept -> lw::Failable<int>;
			static auto s_createBuffer(
				std::string_view name,
				wl_shm* sharedMemory,
				std::uint32_t width, std::uint32_t height
			) noexcept -> lw::Failable<std::pair<lw::Owned<wl_buffer*>, lw::OwnedSpan<std::byte>>>;

			lw::Owned<wl_surface*> m_surface;
			lw::Owned<xdg_surface*> m_xdgSurface;
			lw::Owned<xdg_toplevel*> m_toplevel;
			lw::Owned<wl_buffer*> m_buffer;
			lw::OwnedSpan<std::byte> m_bufferData;
	};
}
