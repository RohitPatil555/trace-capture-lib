#include <coroutine>
#include <taskPool.hpp>

struct Task {
	struct promise_type {
		Task get_return_object() {
			return Task{ std::coroutine_handle<promise_type>::from_promise( *this ) };
		}

		std::suspend_always initial_suspend() { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }

		void return_void() {}
		void unhandled_exception() {}
	};

	std::coroutine_handle<promise_type> handle;

	Task( std::coroutine_handle<promise_type> h ) : handle( h ) {}

	void resume() {
		if ( !handle.done() ) {
			handle.resume();
		}
	}

	bool done() const { return handle.done(); }
};
