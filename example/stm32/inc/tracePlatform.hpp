// SPDX-License-Identifier: MIT | Author: Rohit Patil

#include <trace.hpp>
#include <traceCollector.hpp>
#include <trace_types.hpp>

#pragma once

class Stm32TracePlatform : public tracePlatform {
public:
	uint64_t getTimestamp();
	bool traceTryLock();
	void traceUnlock();
	void packetLock();
	void packetUnlock();
};
