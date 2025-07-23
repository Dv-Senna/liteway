#include "liteway/wayland/instance.hpp"

#include <wayland-client-core.h>

#include "liteway/error.hpp"


namespace lw::wayland {
	Instance::~Instance() {
		if (m_display == nullptr)
			return;
		wl_display_disconnect(m_display.release());
	}


	auto Instance::create(CreateInfos&& createInfos) noexcept -> lw::Failable<Instance> {
		Instance instance {};
		instance.m_display = Owned{wl_display_connect(nullptr)};
		if (instance.m_display == nullptr)
			return lw::makeErrorStack("Can't connect display");
		return instance;
	}
}
