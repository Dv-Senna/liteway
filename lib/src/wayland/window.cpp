#include "liteway/wayland/window.hpp"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <wayland-client-protocol.h>
#include <xdg-shell/xdg-shell-client-protocol.h>

#include "liteway/color.hpp"
#include "liteway/error.hpp"
#include "liteway/janitor.hpp"
#include "liteway/wayland/instance.hpp"


namespace lw::wayland {
	static const xdg_surface_listener xdgSurfaceListener {
		.configure = [](void*, xdg_surface* surface, std::uint32_t serial) noexcept -> void {
			xdg_surface_ack_configure(surface, serial);
		}
	};


	Window::~Window() {
		if (!m_bufferData.empty()) {
			munmap(m_bufferData.data(), m_bufferData.size());
			m_bufferData = lw::OwnedSpan<std::byte> {nullptr, nullptr};
		}
		if (m_buffer != nullptr)
			wl_buffer_destroy(m_buffer.release());
		if (m_toplevel != nullptr)
			xdg_toplevel_destroy(m_toplevel.release());
		if (m_xdgSurface != nullptr)
			xdg_surface_destroy(m_xdgSurface.release());
		if (m_surface != nullptr)
			wl_surface_destroy(m_surface.release());
	}


	auto Window::create(const CreateInfos& createInfos) noexcept -> lw::Failable<Window> {
		Window window {};

		window.m_surface = Owned{wl_compositor_create_surface(createInfos.instance.m_state->compositor)};
		if (window.m_surface == nullptr)
			return lw::makeErrorStack("Can't create wayland surface");

		window.m_xdgSurface = Owned{xdg_wm_base_get_xdg_surface(
			createInfos.instance.m_state->windowManagerBase, window.m_surface
		)};
		if (window.m_xdgSurface == nullptr)
			return lw::makeErrorStack("Can't create xdg surface");

		if (xdg_surface_add_listener(window.m_xdgSurface, &xdgSurfaceListener, nullptr) != 0)
			return lw::makeErrorStack("Can't add listener to xdg surface");

		window.m_toplevel = Owned{xdg_surface_get_toplevel(window.m_xdgSurface)};
		if (window.m_toplevel == nullptr)
			return lw::makeErrorStack("Can't get xdg surface's toplevel");

		lw::Failable bufferWithError {Window::s_createBuffer(
			createInfos.title,
			createInfos.instance.m_state->sharedMemory,
			createInfos.width,
			createInfos.height
		)};
		if (!bufferWithError)
			return lw::pushToErrorStack(bufferWithError, "Can't create buffer for surface");
		auto& [buffer, bufferData] {*bufferWithError};
		window.m_buffer = std::move(buffer);
		window.m_bufferData = std::move(bufferData);

		wl_surface_attach(window.m_surface, window.m_buffer, 0, 0);
		wl_surface_commit(window.m_surface);

		(void)window.fill({.r = 0, .g = 0, .b = 0, .a = 170});
		return window;
	}


	auto Window::fill(const lw::Color& color) noexcept -> lw::Failable<void> {
		assert((m_bufferData.size() & 0b11) == 0b00 && "Buffer size must be a multiple of 4, so it can be uint32_t");
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		assert((reinterpret_cast<std::uintptr_t> (m_bufferData.data()) & 0b11) == 0b00
			&& "Buffer must be aligned to 4 bytes, so it can be uint32_t"
		);
		const std::span<std::uint32_t> bufferDataAsU32 {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			reinterpret_cast<std::uint32_t*> (m_bufferData.data()),
			m_bufferData.size() >> 2uz
		};
		std::ranges::fill(bufferDataAsU32, colorToUint32(color));
		return {};
	}


	auto Window::s_createAnonymousFile(std::string_view name, std::size_t size) noexcept -> lw::Failable<int> {
		using namespace std::string_view_literals;
		const std::string_view postfix {"-liteway-wayland-XXXXXX"};
		const char* directoryPath {std::getenv("XDG_RUNTIME_DIR")};
		if (directoryPath == nullptr)
			return lw::makeErrorStack("Can't create anonymous file '{}' of size {}B", name, size);
		auto path {std::array{std::string_view{directoryPath}, "/"sv, name, postfix}
			| std::views::join
			| std::ranges::to<std::string> ()
		};

		int fd {mkostemp(path.data(), O_CLOEXEC)};
		if (fd < 0) {
			return lw::makeErrorStack("Can't create anonymous file '{}' : {}",
				name, strerror(errno)
			);
		}
		if (unlink(path.c_str()) != 0)
			return lw::makeErrorStack("Can't unlink anonymous file '{}' : {}", name, strerror(errno));
		if (ftruncate(fd, static_cast<std::int32_t> (size)) != 0)
			return lw::makeErrorStack("Can't truncate anonymous file '{}' : {}", name, strerror(errno));
		return fd;
	}


	auto Window::s_createBuffer(
		std::string_view name,
		wl_shm* sharedMemory,
		std::uint32_t width, std::uint32_t height
	) noexcept -> lw::Failable<std::pair<lw::Owned<wl_buffer*>, lw::OwnedSpan<std::byte>>> {
		constexpr auto surfaceFormat {WL_SHM_FORMAT_ARGB8888};
		constexpr std::size_t bytesPerPixel {4uz};

		const std::size_t stride {width * bytesPerPixel};
		const std::size_t size {height * stride};

		lw::Failable fdWithError {Window::s_createAnonymousFile(name, size)};
		if (!fdWithError)
			return lw::pushToErrorStack(fdWithError, "Can't create anonymous file");
		const int fd {*fdWithError};
		lw::Janitor _ {[fd]() noexcept {close(fd);}};

		const auto bufferData {static_cast<std::byte*> (mmap(nullptr, size, PROT_WRITE, MAP_SHARED, fd, 0))};
		if (bufferData == MAP_FAILED)
			return lw::makeErrorStack("Can't map anonymous file : {}", strerror(errno));

		wl_shm_pool* pool {wl_shm_create_pool(sharedMemory, fd, static_cast<std::int32_t> (size))};
		if (pool == nullptr)
			return lw::makeErrorStack("Can't create shared memory pool");
		wl_buffer* buffer {wl_shm_pool_create_buffer(
			pool, 0, static_cast<std::int32_t> (width), static_cast<std::int32_t> (height),
			static_cast<std::int32_t> (stride), surfaceFormat
		)};
		wl_shm_pool_destroy(pool);
		if (buffer == nullptr)
			return lw::makeErrorStack("Can't create buffer from shared memory pool");
		return std::make_pair(lw::Owned{buffer}, lw::OwnedSpan{bufferData, size});
	}
}
