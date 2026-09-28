
#include <array>
#include <span>
#include <stm32/uart.hpp>

typedef enum {
	Control_1 = 0x0c,
	Status	  = 0x00,
	Data	  = 0x04,
} usart_regOffset_e;

typedef enum {
	enable	   = 13,
	farme_size = 12,
	tx_enable  = 3,
	rx_enable  = 2,
} usart_ctrl1_e;

void Uart::initialize() {
	volatile uint32_t *cr1_addr = getRegAddr( usart_regOffset_e::Control_1 );

	constexpr uint32_t value = ( 1 << usart_ctrl1_e::enable ) | ( 1 << usart_ctrl1_e::tx_enable ) |
							   ( 1 << usart_ctrl1_e::rx_enable );

	*cr1_addr = value;
}

bool Uart::send( std::span<const std::byte> data ) {
	volatile uint32_t *dr_addr = getRegAddr( usart_regOffset_e::Data );

	for ( auto val : data ) {
		uint32_t final_value = static_cast<uint32_t>( val );
		*dr_addr			 = final_value;
	}

	return true;
}

bool Uart::recv( std::span<uint8_t> data ) { return true; }
