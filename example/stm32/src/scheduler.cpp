// SPDX-License-Identifier: MIT | Author: Rohit Patil
#include <scheduler.hpp>

#include <tracePlatform.hpp>

bool scheduler::isIdle() {
	bool isIdle = true;
	for ( auto task : taskList ) {
		if ( task != nullptr ) {
			isIdle = false;
		}
	}

	return isIdle;
}

void scheduler::run( void ) {
	Task *currTaskPtr	 = nullptr;
	traceCollector *inst = nullptr;
	Trace<coroutine_t> traceCoroutine;
	coroutine_t *tmsg = nullptr;

	inst = traceCollector::getInstance();
	tmsg = traceCoroutine.getParam();

	for ( size_t i = 0; i < taskCount; i++ ) {
		currTaskPtr = taskList[ i ];

		if ( currTaskPtr == nullptr ) {
			continue;
		}

		if ( currTaskPtr->is_completed() ) {
			taskList[ i ] = nullptr;
			continue;
		}

		tmsg->task_id = i;
		inst->pushTrace( &traceCoroutine );

		currTaskPtr->resume();
	}
}
