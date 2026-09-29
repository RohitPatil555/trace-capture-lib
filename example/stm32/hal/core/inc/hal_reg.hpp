// SPDX-License-Identifier: MIT | Author: Rohit Patil
/**
 * @file hal_reg.hpp
 * @brief Register and bitfield abstraction for hardware access.
 */
#include <concepts>
#include <cstddef>
#include <cstdint>

// --- 1. Access Behavior Tags ---
struct Ro {}; // Read-Only
struct Wo {}; // Write-Only
struct Rw {}; // Read-Write

// --- 2. Concepts for Access Rules ---
template <typename Access>
concept IsReadable = std::same_as<Access, Ro> || std::same_as<Access, Rw>;

template <typename Access>
concept IsWritable = std::same_as<Access, Wo> || std::same_as<Access, Rw>;

template <std::size_t Address, typename Access, bool IsMultiField = false> class Register32 {
	inline static volatile uint32_t *const addr = reinterpret_cast<volatile uint32_t *>( Address );

public:
	[[nodiscard]] constexpr uint32_t read() const
		requires IsReadable<Access>
	{
		return *addr;
	}

	constexpr void write( uint32_t value ) const
		requires IsWritable<Access>
	{
		*addr = value;
	}

	template <uint32_t offset, uint32_t width>
	[[nodiscard]] uint32_t readField() const
		requires( IsReadable<Access> && IsMultiField )
	{
		constexpr uint32_t mask = ( ( 1u << width ) - 1u ) << offset;
		uint32_t current		= 0;

		current = ( read() & mask );
		return ( current >> offset );
	}

	template <uint32_t offset, uint32_t width>
	void writeField( uint32_t value ) const
		requires( IsWritable<Access> && IsMultiField )
	{
		constexpr uint32_t mask = ( ( 1u << width ) - 1u ) << offset;
		uint32_t current		= 0;

		current = ( read() & ~mask );
		current |= ( ( value << offset ) & mask );
		write( current );
	}

	template <uint32_t offset>
	[[nodiscard]] bool checkBit() const
		requires IsReadable<Access>
	{
		return ( read() & ( 0x1 << offset ) ) ? true : false;
	}

	template <uint32_t offset>
	void setBit() const
		requires IsWritable<Access>
	{
		uint32_t value = 0;
		value		   = ( read() | ( 0x1 << offset ) );
		write( value );
	}

	template <uint32_t offset>
	void clearBit() const
		requires IsWritable<Access>
	{
		uint32_t value = 0;
		value		   = ( read() & ~( 0x1 << offset ) );
		write( value );
	}
};

struct RegisterDef {
	uint32_t offset;
	const char *name;
};
