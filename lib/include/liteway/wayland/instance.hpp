#pragma once

#include <wayland-client.h>

#include "liteway/export.hpp"


namespace lw::wayland {
	class LW_EXPORT Instance {
		Instance(const Instance&) = delete;
		auto operator=(const Instance&) = delete;

		public:
			inline Instance() noexcept = default;
			inline Instance(Instance&&) noexcept = default;
			inline auto operator=(Instance&&) noexcept -> Instance& = default;

			struct CreateInfos {

			};

//			static auto create(CreateInfos&& createInfos) noexcept -> ;


		private:
			wl_display* m_display;
	};
}
