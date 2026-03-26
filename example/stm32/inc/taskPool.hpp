#include <bitset>
#include <cstddef>
#include <cstdint>
#include <new>

// ---------------- Object Pool (std::bitset-based) ----------------
template <std::size_t N, std::size_t FrameSize> struct TaskPool {
	alignas( std::max_align_t ) std::uint8_t storage[ N ][ FrameSize ];
	std::bitset<N> used; // 1 = used, 0 = free

	void *allocate( std::size_t size ) {
		if ( size > FrameSize )
			return nullptr;

		for ( std::size_t i = 0; i < N; ++i ) {
			if ( !used.test( i ) ) {
				used.set( i );
				return storage[ i ];
			}
		}
		return nullptr;
	}

	void deallocate( void *ptr ) {
		for ( std::size_t i = 0; i < N; ++i ) {
			if ( storage[ i ] == ptr ) {
				used.reset( i );
				return;
			}
		}
	}
};
