#include <cstdint>
#include <span>

enum class UartError { None, Busy, RecvData, NotAvailable, Unknown };

struct UartConfig {
	uint32_t baudRate;
	uint8_t dataBits;
	bool stopBit;
};

template <typename T>
concept UartDriver_C =
	requires( T drivers, std::span<const uint8_t> tx_data, std::span<uint8_t> rx_data ) {
		{ drivers.init( std::declval<const UartConfig &>() ) } -> std::same_as<UartError>;
		{ drivers.trasmit( tx_data ) } -> std::same_as<UartError>;
		{ drivers.receive( rx_data ) } -> std::same_as<UartError>;
		{ drivers.is_busy() } -> std::same_as<bool>;
		{ drivers.is_recv_data_available() } -> std::same_as<bool>;
	};

template <UartDriver_C Driver> class UartHAL {
	Driver &hw;

public:
	explicit UartHAL( Driver &uartDriver ) : hw( uartDriver ) {}

	auto send( std::span<const uint8_t> data ) -> UartError {
		if ( hw.is_busy() ) {
			return UartError::Busy;
		}

		if ( hw.is_recv_data_available() ) {
			return UartError::RecvData;
		}

		return hw.trasmit( data );
	}

	auto recv( std::span<uint8_t> data ) -> UartError {
		if ( hw.is_busy() ) {
			return UartError::Busy;
		}

		if ( !hw.is_recv_data_available() ) {
			return UartError.NotAvailable;
		}

		return hw.receive( data );
	}

	auto configure( const UartConfig &config ) -> UartError { return hw.init( config ); }
};
