#include <stdint.h>

extern "C" void Reset_Handler() {
	uint32_t count = 0;
	while ( true ) {
		count += 1;
	}
}

// Vector Table
extern "C" uint32_t _estack;
__attribute__( ( section( ".isr_vector" ) ) ) uint32_t vector_table[] = { (uint32_t)&_estack,
																		  (uint32_t)Reset_Handler };
