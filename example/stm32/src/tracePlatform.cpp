#include <tracePlatform.hpp>

static uint64_t g_counter;

uint64_t Stm32TracePlatform::getTimestamp() {
	g_counter += 10;
	return g_counter;
}

bool Stm32TracePlatform::traceTryLock() { return true; }

void Stm32TracePlatform::traceUnlock() {}

void Stm32TracePlatform::packetLock() {}

void Stm32TracePlatform::packetUnlock() {}
