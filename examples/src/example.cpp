#include <cstdlib>
#include <print>

#include <liteway/error.hpp>
#include <liteway/janitor.hpp>
#include <liteway/wayland/instance.hpp>
#include <liteway/wayland/window.hpp>


auto run() noexcept -> lw::Failable<void> {
	lw::Failable instanceWithError {lw::wayland::Instance::create({
		
	})};
	if (!instanceWithError)
		return lw::pushToErrorStack(instanceWithError, "Can't create liteway instance");
	auto& instance {*instanceWithError};

	lw::Failable windowWithError {lw::wayland::Window::create({
		.instance = instance,
		.title = "liteway",
		.width = 16*70, .height = 9*70
	})};
	if (!windowWithError)
		return lw::pushToErrorStack(windowWithError, "Can't create liteway window");
	[[maybe_unused]]
	auto& window {*windowWithError};

	bool running {true};
	while (running) {
		lw::Failable litewayUpdateResult {instance.update()};
		if (!litewayUpdateResult) [[unlikely]]
			return lw::pushToErrorStack(litewayUpdateResult, "Can't update liteway");
	}
	return {};
}

auto main(int, char**) -> int {
	lw::Failable runResult {run()};
	if (runResult)
		return EXIT_SUCCESS;
	runResult.error().print(stderr);
	return EXIT_FAILURE;
}
