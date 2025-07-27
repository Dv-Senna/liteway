#pragma once

#include <memory>
#include <xdg-shell/xdg-shell-client-protocol.h>
#include <wayland-client.h>

#include "liteway/error.hpp"
#include "liteway/export.hpp"
#include "liteway/pointer.hpp"


namespace lw::wayland {
	namespace internals {
		struct InstanceState;

		using SeatListenerUserData = std::pair<InstanceState&, lw::Failable<void>&>;
		struct SharedMemoryListenerUserData {
			std::vector<std::uint32_t> supportedFormats;
		};
		struct RegistryListenerUserData {
			inline RegistryListenerUserData(InstanceState& state) noexcept :
				state {state},
				result {},
				seatListenerUserData {state, result},
				sharedMemoryListenerUserData {}
			{}

			InstanceState& state;
			lw::Failable<void> result;
			SeatListenerUserData seatListenerUserData;
			SharedMemoryListenerUserData sharedMemoryListenerUserData;
		};

		struct InstanceState {
			inline InstanceState() noexcept :
				registryListenerUserData {*this}
			{}

			internals::RegistryListenerUserData registryListenerUserData;
			lw::Owned<wl_display*> display;
			lw::Owned<wl_registry*> registry;
			lw::Owned<wl_compositor*> compositor;
			lw::Owned<xdg_wm_base*> windowManagerBase;
			lw::Owned<wl_shm*> sharedMemory;
			lw::Owned<wl_seat*> seat;
			lw::Owned<wl_pointer*> pointer;
			lw::Owned<wl_keyboard*> keyboard;
		};
	}


	class LW_EXPORT Instance {
		Instance(const Instance&) = delete;
		auto operator=(const Instance&) = delete;
		friend class Window;

		public:
			inline Instance() noexcept = default;
			inline Instance(Instance&&) noexcept = default;
			inline auto operator=(Instance&&) noexcept -> Instance& = default;
			~Instance();

			struct CreateInfos {

			};

			static auto create(CreateInfos&& createInfos) noexcept -> lw::Failable<Instance>;

			auto update() noexcept -> lw::Failable<void>;

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
			static auto handleSharedMemoryFormat(
				void* data,
				wl_shm* sharedMemory,
				std::uint32_t format
			) noexcept -> void;
			static auto handleSeatCapabilites(void* data, wl_seat* seat, std::uint32_t capabilities) noexcept -> void;

		private:
			std::unique_ptr<internals::InstanceState> m_state;
	};
}
