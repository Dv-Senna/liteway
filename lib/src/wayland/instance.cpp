#include "liteway/wayland/instance.hpp"

#include <cassert>

#include <xdg-shell/xdg-shell-client-protocol.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

#include "liteway/error.hpp"
#include "liteway/utils.hpp"


namespace lw::wayland {
	static const wl_registry_listener registryListener {
		.global = &Instance::handleRegistryGlobal,
		.global_remove = [](void*, wl_registry*, std::uint32_t) noexcept -> void {}
	};

	static const xdg_wm_base_listener windowManagerBaseListener {
		.ping = [](void*, xdg_wm_base* windowManagerBase, std::uint32_t serial) noexcept -> void {
			xdg_wm_base_pong(windowManagerBase, serial);
		}
	};

	static const wl_shm_listener sharedMemoryListener {
		.format = &Instance::handleSharedMemoryFormat
	};

	static const wl_seat_listener seatListener {
		.capabilities = &Instance::handleSeatCapabilites,
		.name = [](void*, wl_seat*, const char*) noexcept -> void {}
	};


	Instance::~Instance() {
		if (!m_state)
			return;
		if (m_state->keyboard != nullptr)
			wl_keyboard_destroy(m_state->keyboard.release());
		if (m_state->pointer != nullptr)
			wl_pointer_destroy(m_state->pointer.release());
		if (m_state->seat != nullptr)
			wl_seat_destroy(m_state->seat.release());
		if (m_state->sharedMemory != nullptr)
			wl_shm_destroy(m_state->sharedMemory.release());
		if (m_state->windowManagerBase != nullptr)
			xdg_wm_base_destroy(m_state->windowManagerBase.release());
		if (m_state->compositor != nullptr)
			wl_compositor_destroy(m_state->compositor.release());
		if (m_state->registry != nullptr)
			wl_registry_destroy(m_state->registry.release());
		if (m_state->display != nullptr)
			wl_display_disconnect(m_state->display.release());
	}


	auto Instance::create([[maybe_unused]] CreateInfos&& createInfos) noexcept -> lw::Failable<Instance> {
		static std::size_t instanceCount {};
		assert(++instanceCount == 1 && "You can't create more than one instance of liteway");
		Instance instance {};
		instance.m_state = std::make_unique<internals::InstanceState> ();
		instance.m_state->display = lw::Owned{wl_display_connect(nullptr)};
		if (instance.m_state->display == nullptr)
			return lw::makeErrorStack("Can't connect display");

		instance.m_state->registry = lw::Owned{wl_display_get_registry(instance.m_state->display)};
		if (instance.m_state->registry == nullptr)
			return lw::makeErrorStack("Can't get registry");

		if (wl_registry_add_listener(
			instance.m_state->registry,
			&registryListener,
			&instance.m_state->registryListenerUserData
		) != 0)
			return lw::makeErrorStack("Can't add listener to registry");

		if (wl_display_dispatch(instance.m_state->display) < 0)
			return lw::makeErrorStack("Can't dispatch display event queue");
		if (wl_display_roundtrip(instance.m_state->display) < 0)
			return lw::makeErrorStack("Can't wait for display roundtrip");

		if (!instance.m_state->registryListenerUserData.result) {
			return lw::pushToErrorStack(instance.m_state->registryListenerUserData.result,
				"Can't bind stuff to the global registry"
			);
		}
		instance.m_state->registryListenerUserData.result = {};

		auto& supportedFormats {instance.m_state->registryListenerUserData.sharedMemoryListenerUserData.supportedFormats};
		if (std::ranges::find(supportedFormats, WL_SHM_FORMAT_ARGB8888) == supportedFormats.end())
			return lw::makeErrorStack("Needed shared memory format 'WL_SHM_FORMAT_ARGB8888' is not supported");
		return instance;
	}


	auto Instance::update() noexcept -> lw::Failable<void> {
		if (wl_display_dispatch(m_state->display) < 0)
			return lw::makeErrorStack("Can't dispatch display");
		return {};
	}


	template <>
	auto Instance::bindGlobalFromRegistry<wl_compositor> (
		internals::RegistryListenerUserData& registryListenerUserData,
		std::uint32_t name,
		std::uint32_t version
	) noexcept -> lw::Failable<void> {
		internals::InstanceState& state {registryListenerUserData.state};
		state.compositor = lw::Owned{static_cast<wl_compositor*> (
			wl_registry_bind(state.registry, name, &wl_compositor_interface, version)
		)};
		if (state.compositor == nullptr)
			return lw::makeErrorStack("Can't bind compositor");
		return {};
	}


	template <>
	auto Instance::bindGlobalFromRegistry<xdg_wm_base> (
		internals::RegistryListenerUserData& registryListenerUserData,
		std::uint32_t name,
		std::uint32_t version
	) noexcept -> lw::Failable<void> {
		internals::InstanceState& state {registryListenerUserData.state};
		state.windowManagerBase = lw::Owned{static_cast<xdg_wm_base*> (
			wl_registry_bind(state.registry, name, &xdg_wm_base_interface, version)
		)};
		if (state.windowManagerBase == nullptr)
			return lw::makeErrorStack("Can't bind xdg window manager base");

		if (xdg_wm_base_add_listener(state.windowManagerBase, &windowManagerBaseListener, nullptr) != 0)
			return lw::makeErrorStack("Can't add listener to xdg window manager base");
		return {};
	}


	template <>
	auto Instance::bindGlobalFromRegistry<wl_shm> (
		internals::RegistryListenerUserData& registryListenerUserData,
		std::uint32_t name,
		std::uint32_t version
	) noexcept -> lw::Failable<void> {
		internals::InstanceState& state {registryListenerUserData.state};
		state.sharedMemory = lw::Owned{static_cast<wl_shm*> (
			wl_registry_bind(state.registry, name, &wl_shm_interface, version)
		)};
		if (state.sharedMemory == nullptr)
			return lw::makeErrorStack("Can't bind shared memory");

		if (wl_shm_add_listener(
			state.sharedMemory,
			&sharedMemoryListener,
			&registryListenerUserData.sharedMemoryListenerUserData
		) != 0)
			return lw::makeErrorStack("Can't add listener to shared memory");
		return {};
	}


	template <>
	auto Instance::bindGlobalFromRegistry<wl_seat> (
		internals::RegistryListenerUserData& registryListenerUserData,
		std::uint32_t name,
		std::uint32_t version
	) noexcept -> lw::Failable<void> {
		internals::InstanceState& state {registryListenerUserData.state};
		state.seat = lw::Owned{static_cast<wl_seat*> (
			wl_registry_bind(state.registry, name, &wl_seat_interface, version)
		)};
		if (state.seat == nullptr)
			return lw::makeErrorStack("Can't bind seat");

		if (wl_seat_add_listener(state.seat, &seatListener, &registryListenerUserData.seatListenerUserData) != 0)
			return lw::makeErrorStack("Can't add listener to seat");
		return {};
	}


	auto Instance::handleRegistryGlobal(
		void* data,
		[[maybe_unused]] wl_registry* registry,
		std::uint32_t name,
		const char* interface,
		std::uint32_t version
	) noexcept -> void {
		using namespace std::string_view_literals;
		auto& registryListenerUserData {*static_cast<internals::RegistryListenerUserData*> (data)};
		if (!registryListenerUserData.result)
			return;

		using Interfaces = std::tuple<wl_compositor, xdg_wm_base, wl_shm, wl_seat>;

		registryListenerUserData.result = [&] <std::size_t I = 0> (this const auto& self) noexcept
			-> lw::Failable<void>
		{
			if (interface == lw::utils::getTypeName<std::tuple_element_t<I, Interfaces>> ()) {
				return bindGlobalFromRegistry<std::tuple_element_t<I, Interfaces>> (
					registryListenerUserData,
					name,
					version
				);
			}
			if constexpr (I < std::tuple_size_v<Interfaces> - 1)
				return self.template operator() <I+1> ();
			return {};
		} ();
	}


	auto Instance::handleSharedMemoryFormat(
		void* data,
		[[maybe_unused]] wl_shm* sharedMemory,
		std::uint32_t format
	) noexcept -> void {
		auto& sharedMemoryUserData {*static_cast<internals::SharedMemoryListenerUserData*> (data)};
		sharedMemoryUserData.supportedFormats.push_back(format);
	}


	auto Instance::handleSeatCapabilites(void* data, wl_seat* seat, std::uint32_t capabilities) noexcept -> void {
		auto& seatListenerUserData {*static_cast<internals::SeatListenerUserData*> (data)};
		internals::InstanceState& state {seatListenerUserData.first};
		lw::Failable<void>& result {seatListenerUserData.second};

		if (!(capabilities & WL_SEAT_CAPABILITY_POINTER))
			return (void)(result = lw::makeErrorStack("Can't use seat without pointer capability"));
		if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD))
			return (void)(result = lw::makeErrorStack("Can't use seat without keyboard capability"));

		state.pointer = lw::Owned{wl_seat_get_pointer(seat)};
		if (state.pointer == nullptr)
			return (void)(result = lw::makeErrorStack("Can't get seat pointer"));
		state.keyboard = lw::Owned{wl_seat_get_keyboard(seat)};
		if (state.keyboard == nullptr)
			return (void)(result = lw::makeErrorStack("Can't get seat keyboard"));
	}
}
