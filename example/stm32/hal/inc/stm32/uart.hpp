// SPDX-License-Identifier: MIT | Author: Rohit Patil
#pragma once

#include <hal_reg.hpp>
#include <span>

#define USART1_ADDR ( 0x40013800 )

class Uart {
	volatile uint8_t *addr;

	inline volatile uint32_t *getRegAddr( uint32_t offset ) {
		return reinterpret_cast<volatile uint32_t *>( addr + offset );
	}

public:
	Uart( uintptr_t _addr ) { addr = reinterpret_cast<volatile uint8_t *>( _addr ); }
	~Uart() = default;

	void initialize();
	bool send( std::span<const std::byte> data );
	bool recv( std::span<uint8_t> data );
};
