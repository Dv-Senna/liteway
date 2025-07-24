#pragma once

#include <memory>
#include <xdg-shell/xdg-shell-client-protocol.h>
#include <wayland-client.h>

#include "liteway/error.hpp"
#include "liteway/export.hpp"
#include "liteway/pointer.hpp"


namespace lw::wayland {
	class Instance;

	namespace internals {
		using SeatListenerUserData = std::pair<Instance&, lw::Failable<void>&>;
		struct RegistryListenerUserData {
			inline RegistryListenerUserData(Instance& instance) noexcept :
				instance {instance},
				result {},
				seatListenerUserData {instance, result}
			{}

			Instance& instance;
			lw::Failable<void> result;
			SeatListenerUserData seatListenerUserData;
		};
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
				internals::RegistryListenerUserData& registryListenerUserData,
				std::uint32_t name,
				std::uint32_t version
			) noexcept -> lw::Failable<void>;

			static auto handleRegistryGlobal(
				void* data,
				wl_registry* registry,
				std::uint32_t name,
				const char* interface,
				std::uint32_t version
			) noexcept -> void;

			static auto handleSeatCapabilites(void* data, wl_seat* seat, std::uint32_t capabilities) noexcept -> void;

		private:
			std::unique_ptr<internals::RegistryListenerUserData> m_registryListenerUserData;
			lw::Owned<wl_display*> m_display;
			lw::Owned<wl_registry*> m_registry;
			lw::Owned<wl_compositor*> m_compositor;
			lw::Owned<xdg_wm_base*> m_windowManagerBase;
			lw::Owned<wl_seat*> m_seat;
			lw::Owned<wl_pointer*> m_pointer;
			lw::Owned<wl_keyboard*> m_keyboard;
	};
}
