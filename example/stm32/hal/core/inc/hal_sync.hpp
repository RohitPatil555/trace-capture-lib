
#pragma once

#include <cstdint>
#include <span>

typedef enum {
	halStatus_Success,
	halStatus_InitFail,
	halStatus_TxFail,
	halStatus_RxFail
} HalSyncStatus_e;

template <typename T>
concept DeviceSync = requires( T t, std::span<uint8_t> txData, std::span<uint8_t> rxData ) {
	{ t.initialize() } -> std::same_as<HalSyncStatus_e>;
	{ t.send( txData ) } -> std::same_as<HalSyncStatus_e>;
	{ t.recv( rxData ) } -> std::same_as<HalSyncStatus_e>;
};

template <DeviceSync DevSyncT> class HalSyncDevice {
	DevSyncT _device;

public:
	HalSyncDevice()	 = default;
	~HalSyncDevice() = default;

	inline HalSyncStatus_e init() { return _device.initialize(); }

	inline HalSyncStatus_e send( std::span<uint8_t> &data ) { return _device.send( &data ); }

	inline HalSyncStatus_e recv( std::span<uint8_t> &data ) { return _device.recv( &data ); }
};
