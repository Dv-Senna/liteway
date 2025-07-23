#include "liteway/wayland/instance.hpp"

#include <cassert>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

#include "liteway/error.hpp"
#include "liteway/utils.hpp"


namespace lw::wayland {
	static const wl_registry_listener registryListener {
		.global = &Instance::handleRegistryGlobal,
		.global_remove = [](void*, wl_registry*, std::uint32_t) -> void {}
	};


	Instance::~Instance() {
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

		if (wl_registry_add_listener(instance.m_registry, &registryListener, &instance) != 0)
			return lw::makeErrorStack("Can't add listener to registry");

		if (wl_display_dispatch(instance.m_display) < 0)
			return lw::makeErrorStack("Can't dispatch display event queue");
		if (wl_display_roundtrip(instance.m_display) < 0)
			return lw::makeErrorStack("Can't wait for display roundtrip");

		if (instance.m_compositor == nullptr)
			return lw::makeErrorStack("Can't bind compositor");
		return instance;
	}


	template <>
	auto Instance::bindGlobalFromRegistry<wl_compositor> (
		Instance& instance,
		std::uint32_t name,
		std::uint32_t version
	) noexcept -> void {
		instance.m_compositor = lw::Owned{reinterpret_cast<wl_compositor*> (
			wl_registry_bind(instance.m_registry, name, &wl_compositor_interface, version)
		)};
	}


	auto Instance::handleRegistryGlobal(
		void* data,
		[[maybe_unused]] wl_registry* registry,
		std::uint32_t name,
		const char* interface,
		std::uint32_t version
	) -> void {
		using namespace std::string_view_literals;
		auto& instance {*reinterpret_cast<Instance*> (data)};

		using Interfaces = std::tuple<wl_compositor>;

		[&] <std::size_t I = 0> (this const auto& self) {
			if (interface == lw::utils::getTypeName<std::tuple_element_t<I, Interfaces>> ())
				return bindGlobalFromRegistry<std::tuple_element_t<I, Interfaces>> (instance, name, version);
			if constexpr (I < std::tuple_size_v<Interfaces> - 1)
				self.template operator() <I+1> ();
		} ();
	}
}
