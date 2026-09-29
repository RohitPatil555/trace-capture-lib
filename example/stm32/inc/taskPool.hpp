// SPDX-License-Identifier: MIT | Author: Rohit Patil
#include <cstddef>
#include <cstdint>
#include <new>

#pragma once

class TaskPool {
public:
	virtual void *allocate( std::size_t size ) = 0;
	virtual void deallocate( void *ptr )	   = 0;
};

// ---------------- Object Pool (std::bitset-based) ----------------
template <std::size_t N, std::size_t FrameSize> class FixTaskFrameAllocator : public TaskPool {
	alignas( std::max_align_t ) std::uint8_t storage[ N ][ FrameSize ];
	uint8_t used[ ( ( N / 8 ) + 1 ) ]; // 1 = used, 0 = free

public:
	virtual void *allocate( std::size_t size ) noexcept {
		uint8_t byteOffset = 0;
		uint8_t bitOffset  = 0;

		if ( size > FrameSize )
			return nullptr;

		for ( std::size_t i = 0; i < N; ++i ) {
			byteOffset = (uint8_t)( i / 8 );
			bitOffset  = (uint8_t)( i % 8 );
			if ( !( used[ byteOffset ] & ( 1 << bitOffset ) ) ) {
				used[ byteOffset ] |= ( 1 << bitOffset );
				return &storage[ i ];
			}
		}
		return nullptr;
	}

	virtual void deallocate( void *ptr ) noexcept {
		uint8_t byteOffset = 0;
		uint8_t bitOffset  = 0;

		for ( std::size_t i = 0; i < N; ++i ) {
			byteOffset = (uint8_t)( i / 8 );
			bitOffset  = (uint8_t)( i % 8 );
			if ( &storage[ i ] == ptr ) {
				used[ byteOffset ] &= ( ~( 1 << bitOffset ) );
				return;
			}
		}
	}
};
