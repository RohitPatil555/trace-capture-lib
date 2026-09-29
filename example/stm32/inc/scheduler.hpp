// SPDX-License-Identifier: MIT | Author: Rohit Patil
#include <cstdint>
#include <task.hpp>

#pragma once

#define SCHEDULER_TASK_COUNT_MAX 3
#define SCHEDULER_TASK_FRAM_SIZE 128

class scheduler {
	Task *taskList[ SCHEDULER_TASK_COUNT_MAX ];
	uint32_t taskCount;

public:
	scheduler() {
		taskCount = 0;
		for ( size_t i = 0; i < SCHEDULER_TASK_COUNT_MAX; i++ ) {
			taskList[ i ] = nullptr;
		}
	}

	~scheduler() {
		taskCount = 0;
		for ( size_t i = 0; i < SCHEDULER_TASK_COUNT_MAX; i++ ) {
			taskList[ i ] = nullptr;
		}
	}

	bool append( Task *ptr ) {
		if ( taskCount >= SCHEDULER_TASK_COUNT_MAX ) {
			return false;
		}

		taskList[ taskCount ] = ptr;
		taskCount++;
		return true;
	}

	bool isIdle();
	void run( void );
};
