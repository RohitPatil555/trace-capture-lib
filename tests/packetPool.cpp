// SPDX-License-Identifier: MIT | Author: Rohit Patil
#include <gtest/gtest.h>

#include <staticPool.hpp>

using namespace std;

typedef struct {
	uint32_t id;
	array<uint8_t, 32> val;
} __attribute__( ( packed ) ) pktPayload_t;

TEST( StaticPoolTest, testPoolAllocate ) {
	pktPayload_t *ptr = nullptr;
	StaticPool<pktPayload_t, 2> sp;

	ptr = sp.allocate();
	EXPECT_NE( ptr, nullptr );

	ptr = sp.allocate();
	EXPECT_NE( ptr, nullptr );

	ptr = sp.allocate();
	EXPECT_EQ( ptr, nullptr );

	EXPECT_EQ( sp.usedCount(), 2 );
}

TEST( StaticPoolTest, testPoolRelease ) {
	pktPayload_t *ptr = nullptr;
	StaticPool<pktPayload_t, 2> sp;
	vector<pktPayload_t *> ptr_list;

	ptr = sp.allocate();
	EXPECT_NE( ptr, nullptr );
	ptr_list.push_back( ptr );

	ptr = sp.allocate();
	EXPECT_NE( ptr, nullptr );
	ptr_list.push_back( ptr );

	ptr = sp.allocate();
	EXPECT_EQ( ptr, nullptr );

	for ( auto &_p : ptr_list ) {
		sp.release( _p );
	}

	EXPECT_EQ( sp.usedCount(), 0 );
}
