#include <coroutine>
#include <taskPool.hpp>

#pragma once

struct Task {
	struct promise_type {
		static TaskPool *poolPtr;

		Task get_return_object() {
			return Task( std::coroutine_handle<promise_type>::from_promise( *this ) );
		}

		std::suspend_always initial_suspend() noexcept { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }
		void unhandled_exception() noexcept {}
		void return_void() noexcept {}

		void *operator new( std::size_t size ) noexcept {
			if ( poolPtr != nullptr ) {
				return poolPtr->allocate( size );
			}

			return nullptr;
		}

		void operator delete( void *ptr ) noexcept {
			if ( poolPtr != nullptr ) {
				poolPtr->deallocate( ptr );
			}
		}

		static Task get_return_object_on_allocation_failure() noexcept { return Task(); }
	};

	std::coroutine_handle<promise_type> handle;

	Task() { handle = nullptr; }

	Task( std::coroutine_handle<promise_type> h ) { handle = h; }

	void resume() {
		if ( handle ) {
			if ( !handle.done() ) {
				handle.resume();
			}
		}
	}

	bool is_completed() {
		if ( handle.done() ) {
			return true;
		}

		return false;
	}

	static void set_task_pool( TaskPool *ptr ) { promise_type::poolPtr = ptr; }
};
