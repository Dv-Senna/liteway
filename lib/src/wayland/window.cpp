#include "liteway/wayland/window.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <wayland-client-protocol.h>
#include <xdg-shell/xdg-shell-client-protocol.h>

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


	auto Window::create(CreateInfos&& createInfos) noexcept -> lw::Failable<Window> {
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

		wl_surface_attach(window.m_surface, window.m_buffer, 0, 0);
		wl_surface_commit(window.m_surface);
		return window;
	}


	auto Window::s_createAnonymousFile(std::string_view name, std::size_t size) noexcept -> lw::Failable<int> {
		using namespace std::string_view_literals;
		const std::string_view postfix {"-liteway-wayland-XXXXXX"};
		const std::string_view directoryPath {std::getenv("XDG_RUNTIME_DIR")};
		if (directoryPath.empty())
			return lw::makeErrorStack("Can't create anonymous file '{}' of size {}B", name, size);
		auto path {std::array{directoryPath, "/"sv, name, postfix}
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
		if (ftruncate(fd, size) != 0)
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

		const auto bufferData {reinterpret_cast<std::byte*> (mmap(nullptr, size, PROT_WRITE, MAP_SHARED, fd, 0))};
		if (bufferData == MAP_FAILED)
			return lw::makeErrorStack("Can't map anonymous file : {}", strerror(errno));

		wl_shm_pool* pool {wl_shm_create_pool(sharedMemory, fd, size)};
		if (pool == nullptr)
			return lw::makeErrorStack("Can't create shared memory pool");
		wl_buffer* buffer {wl_shm_pool_create_buffer(pool, 0, width, height, stride, surfaceFormat)};
		wl_shm_pool_destroy(pool);
		if (buffer == nullptr)
			return lw::makeErrorStack("Can't create buffer from shared memory pool");
		return std::make_pair(lw::Owned{buffer}, lw::OwnedSpan{bufferData, size});
	}
}
