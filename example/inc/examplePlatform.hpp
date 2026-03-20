// SPDX-License-Identifier: MIT | Author: Rohit Patil

#include <trace.hpp>
#include <traceCollector.hpp>
#include <trace_types.hpp>

#include <mutex>

#pragma once

class TestPlatform : public tracePlatform {
	std::mutex traceMutex;
	std::mutex packetMutex;

public:
	uint64_t getTimestamp();
	bool traceTryLock();
	void traceUnlock();
	void packetLock();
	void packetUnlock();
};
