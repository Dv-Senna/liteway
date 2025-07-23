#include <cstdlib>
#include <print>

#include <liteway/error.hpp>
#include <liteway/janitor.hpp>
#include <liteway/wayland/instance.hpp>


auto run() noexcept -> lw::Failable<void> {
	lw::Failable instanceWithError {lw::wayland::Instance::create({
		
	})};
	if (!instanceWithError)
		return lw::pushToErrorStack(instanceWithError, "Can't create liteway instance");

	return {};
}

auto main(int, char**) -> int {
	lw::Failable runResult {run()};
	if (runResult)
		return EXIT_SUCCESS;
	runResult.error().print(stderr);
	return EXIT_FAILURE;
}
