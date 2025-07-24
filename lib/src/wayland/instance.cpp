#include "liteway/wayland/instance.hpp"

#include <cassert>

#include <xdg-shell/xdg-shell-client-protocol.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

#include "liteway/error.hpp"
#include "liteway/utils.hpp"


namespace lw::wayland {
	using RegistryListenerUserData = std::pair<Instance&, lw::Failable<void>>;
	static const wl_registry_listener registryListener {
		.global = &Instance::handleRegistryGlobal,
		.global_remove = [](void*, wl_registry*, std::uint32_t) -> void {}
	};

	static const xdg_wm_base_listener windowManagerBaseListener {
		.ping = [](void*, xdg_wm_base* windowManagerBase, std::uint32_t serial) -> void {
			xdg_wm_base_pong(windowManagerBase, serial);
		}
	};


	Instance::~Instance() {
		if (m_windowManagerBase != nullptr)
			xdg_wm_base_destroy(m_windowManagerBase.release());
		if (m_compositor != nullptr)
			wl_compositor_destroy(m_compositor.release());
		if (m_registry != nullptr)
			wl_registry_destroy(m_registry.release());
		if (m_display != nullptr)
			wl_display_disconnect(m_display.release());
	}


	auto Instance::create(CreateInfos&& createInfos) noexcept -> lw::Failable<Instance> {
		static std::size_t instanceCount {};
		assert(++instanceCount == 1 && "You can't create more than one instance of liteway");
		Instance instance {};
		instance.m_display = lw::Owned{wl_display_connect(nullptr)};
		if (instance.m_display == nullptr)
			return lw::makeErrorStack("Can't connect display");

		instance.m_registry = lw::Owned{wl_display_get_registry(instance.m_display)};
		if (instance.m_registry == nullptr)
			return lw::makeErrorStack("Can't get registry");

		RegistryListenerUserData registryListenerUserData {instance, {}};
		if (wl_registry_add_listener(instance.m_registry, &registryListener, &registryListenerUserData) != 0)
			return lw::makeErrorStack("Can't add listener to registry");

		if (wl_display_dispatch(instance.m_display) < 0)
			return lw::makeErrorStack("Can't dispatch display event queue");
		if (wl_display_roundtrip(instance.m_display) < 0)
			return lw::makeErrorStack("Can't wait for display roundtrip");

		if (!registryListenerUserData.second)
			return lw::pushToErrorStack(registryListenerUserData.second, "Can't bind stuff to the global registry");
		return instance;
	}


	template <>
	auto Instance::bindGlobalFromRegistry<wl_compositor> (
		Instance& instance,
		std::uint32_t name,
		std::uint32_t version
	) noexcept -> lw::Failable<void> {
		instance.m_compositor = lw::Owned{reinterpret_cast<wl_compositor*> (
			wl_registry_bind(instance.m_registry, name, &wl_compositor_interface, version)
		)};
		if (instance.m_compositor == nullptr)
			return lw::makeErrorStack("Can't bind compositor");
		return {};
	}


	template <>
	auto Instance::bindGlobalFromRegistry<xdg_wm_base> (
		Instance& instance,
		std::uint32_t name,
		std::uint32_t version
	) noexcept -> lw::Failable<void> {
		instance.m_windowManagerBase = lw::Owned{reinterpret_cast<xdg_wm_base*> (
			wl_registry_bind(instance.m_registry, name, &xdg_wm_base_interface, version)
		)};
		if (instance.m_windowManagerBase == nullptr)
			return lw::makeErrorStack("Can't bind xdg window manager base");

		if (xdg_wm_base_add_listener(instance.m_windowManagerBase, &windowManagerBaseListener, nullptr) != 0)
			return lw::makeErrorStack("Can't add listener to xdg window manager base");
		return {};
	}


	auto Instance::handleRegistryGlobal(
		void* data,
		[[maybe_unused]] wl_registry* registry,
		std::uint32_t name,
		const char* interface,
		std::uint32_t version
	) -> void {
		using namespace std::string_view_literals;
		auto& registryListenerUserData {*reinterpret_cast<RegistryListenerUserData*> (data)};
		Instance& instance {registryListenerUserData.first};
		lw::Failable<void>& result {registryListenerUserData.second};
		if (!result)
			return;

		using Interfaces = std::tuple<wl_compositor, xdg_wm_base>;

		result = [&] <std::size_t I = 0> (this const auto& self) noexcept -> lw::Failable<void> {
			if (interface == lw::utils::getTypeName<std::tuple_element_t<I, Interfaces>> ())
				return bindGlobalFromRegistry<std::tuple_element_t<I, Interfaces>> (instance, name, version);
			if constexpr (I < std::tuple_size_v<Interfaces> - 1)
				return self.template operator() <I+1> ();
			return {};
		} ();
	}
}
