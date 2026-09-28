#include <compiler_req_apis.h>

void *memset( void *s, int c, size_t n ) {
	uint8_t *p	  = (uint8_t *)( s );
	uint8_t value = (uint8_t)( c );

	while ( n-- ) {
		*p++ = value;
	}
	return s;
}

void *memcpy( void *dest, const void *src, size_t n ) {
	unsigned char *d	   = (unsigned char *)( dest );
	const unsigned char *s = (const unsigned char *)( src );
	while ( n-- ) {
		*d++ = *s++;
	}
	return dest;
}

void abort( void ) {
	// Disable global interrupts to freeze the system safely
	__asm__ volatile( "cpsid i" );

	// Optional: Add a breakpoint or toggle an LED here for debugging
	// __asm__ volatile ("bkpt #0");

	// Infinite loop to halt execution
	while ( 1 ) {
		// Optionally feed a hardware watchdog here if enabled
	}
}

void __assert_func( const char *file, int line, const char *func, const char *failedexpr ) {
	// Handle your assertion failure here (e.g., disable interrupts, log, and infinite loop)
	while ( 1 )
		;
}
