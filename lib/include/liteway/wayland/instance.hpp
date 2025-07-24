#pragma once

#include <xdg-shell/xdg-shell-client-protocol.h>
#include <wayland-client.h>

#include "liteway/error.hpp"
#include "liteway/export.hpp"
#include "liteway/pointer.hpp"


namespace lw::wayland {
	namespace internals {
	}

	class LW_EXPORT Instance {
		Instance(const Instance&) = delete;
		auto operator=(const Instance&) = delete;

		public:
			inline Instance() noexcept = default;
			inline Instance(Instance&&) noexcept = default;
			inline auto operator=(Instance&&) noexcept -> Instance& = default;
			~Instance();

			struct CreateInfos {

			};

			static auto create(CreateInfos&& createInfos) noexcept -> lw::Failable<Instance>;

			template <typename T>
			static auto bindGlobalFromRegistry(
				Instance& instance,
				std::uint32_t name,
				std::uint32_t version
			) noexcept -> lw::Failable<void>;

			static auto handleRegistryGlobal(
				void* data,
				wl_registry* registry,
				std::uint32_t name,
				const char* interface,
				std::uint32_t version
			) -> void;

		private:
			lw::Owned<wl_display*> m_display;
			lw::Owned<wl_registry*> m_registry;
			lw::Owned<wl_compositor*> m_compositor;
			lw::Owned<xdg_wm_base*> m_windowManagerBase;
	};
}
