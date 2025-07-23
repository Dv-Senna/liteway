#pragma once

#include <wayland-client.h>

#include "liteway/error.hpp"
#include "liteway/export.hpp"
#include "liteway/pointer.hpp"


namespace lw::wayland {
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


		private:
			lw::Owned<wl_display*> m_display;
	};
}
