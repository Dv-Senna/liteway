#include <cstdlib>
#include <print>

#include <liteway/instance.hpp>
#include <liteway/error.hpp>


auto foo() -> lw::Failable<int> {
	return lw::makeErrorStack("Error : {}", 12);
}

auto bar() -> lw::Failable<float> {
	auto fooResult {foo()};
	if (!fooResult)
		return lw::pushToErrorStack(fooResult, "bar error : {}", 3.1415f);
	return -1.f;
}


auto main(int, char**) -> int {
	auto barResult {bar()};
	if (!barResult) {
		for (; !barResult.error().isEmpty(); barResult.error().pop()) {
			auto& frame {barResult.error().top()};
			std::println(stderr, "Error in {} ({}:{}) > {}",
				frame.sourceLocation.function_name(),
				frame.sourceLocation.file_name(),
				frame.sourceLocation.column(),
				frame.message
			);
		}
	}

	return EXIT_SUCCESS;
}
