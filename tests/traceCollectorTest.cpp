// SPDX-License-Identifier: MIT | Author: Rohit Patil
#include <gtest/gtest.h>
#include <trace.hpp>
#include <traceCollector.hpp>

using namespace std;

// Mock trace class for testing
typedef struct {
	array<std::byte, 10> value;
} __attribute__( ( packed ) ) mock_trace_t;

template <> struct TraceId<mock_trace_t> {
	static constexpr uint32_t value = 1;
};

class TestPlatform : public tracePlatform {
	uint64_t ts;

public:
	TestPlatform() { ts = 0; }

	uint64_t getTimestamp() {
		ts += 100;
		return ts;
	}

	bool traceTryLock() { return true; }
	void traceUnlock() {}
};

static TestPlatform g_TestPltf;
bool g_isInitialized = false;

class TraceCollectorTest : public ::testing::Test {
	std::vector<std::byte> buffer;

protected:
	traceCollector *inst;

	void SetUp() override {
		size_t bufferSize = traceCollector::getPacketBufferSize( 2 );

		buffer.clear();
		buffer.resize( bufferSize );

		inst = new traceCollector( std::span<std::byte>( buffer.data(), buffer.size() ) );

		inst->setStreamId( 0 );
		inst->setPlatformIntf( &g_TestPltf );
		g_isInitialized = true;
	}

	void TearDown() override {
		delete inst;
		buffer.clear();
	}
};

// Test: Push traces and send first packet
TEST_F( TraceCollectorTest, PushAndSendFirstPacket ) {
	auto *collector = inst;

	// Create mock traces
	auto evt			= new Trace<mock_trace_t>;
	mock_trace_t *param = evt->getParam();
	memset( param->value.data(), 0x11, 10 );

	// Push traces to fill current packet
	for ( int i = 0; i < CONFIG_TRACE_MAX_PER_PACKET; i++ ) {
		collector->pushTrace( evt );
	}

	// as packet size not full force sync
	collector->forceSync();

	delete evt;

	// Verify packet can be sent
	auto sendPacket = collector->getSendPacket();
	EXPECT_TRUE( sendPacket.has_value() );
	EXPECT_FALSE( sendPacket.value().empty() );

	// Complete the packet
	collector->sendPacketCompleted();
}

// Test: Handle empty send queue
TEST_F( TraceCollectorTest, EmptySendQueue ) {
	auto *collector = inst;

	auto sendPacket = collector->getSendPacket();
	EXPECT_FALSE( sendPacket.has_value() );
}

// Test: Sequential packet sending with completion
TEST_F( TraceCollectorTest, SequentialPackets ) {
	auto *collector = inst;

	// Create mock traces
	auto evt1			 = new Trace<mock_trace_t>;
	mock_trace_t *param1 = evt1->getParam();
	memset( param1->value.data(), 0x11, 10 );

	// Push traces to fill current packet
	for ( int i = 0; i < CONFIG_TRACE_MAX_PER_PACKET; i++ ) {
		collector->pushTrace( evt1 );
	}

	// as event size can't full packet, mark as force sync.
	collector->forceSync();

	delete evt1;

	// Get first packet
	auto span1 = collector->getSendPacket();
	EXPECT_TRUE( span1.has_value() );
	vector<byte> data1( span1.value().data(), span1.value().data() + span1.value().size() );

	// Complete first packet
	collector->sendPacketCompleted();

	// Create mock traces
	auto evt2			 = new Trace<mock_trace_t>;
	mock_trace_t *param2 = evt2->getParam();
	memset( param2->value.data(), 0x22, 10 );

	// Push traces to fill current packet
	for ( int i = 0; i < CONFIG_TRACE_MAX_PER_PACKET; i++ ) {
		collector->pushTrace( evt2 );
	}

	// as event size can't full packet, mark as force sync.
	collector->forceSync();

	delete evt2;

	// Get second packet and verify it's different
	auto span2 = collector->getSendPacket();
	EXPECT_TRUE( span2.has_value() );
	vector<byte> data2( span2.value().data(), span2.value().data() + span2.value().size() );

	// Ensure spans are for different packet. Skip 20 byte header size.
	for ( size_t i = 48; i < ( 48 + 10 ); i++ ) {
		EXPECT_NE( data1[ i ], data2[ i ] );
	}
}
