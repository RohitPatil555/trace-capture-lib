#include <cstdint>
#include <task.hpp>

using namespace std;

Task blink_task() {
	uint32_t count = 0;
	while ( 1 ) {
		count++;
	}
}

extern "C" void Reset_Handler() {
	auto task = blink_task();

	task.resume();
}

// Vector Table
extern "C" uint32_t _estack;
__attribute__( ( section( ".isr_vector" ) ) ) uint32_t vector_table[] = { (uint32_t)&_estack,
																		  (uint32_t)Reset_Handler };
