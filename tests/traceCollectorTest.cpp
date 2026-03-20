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

	void packetLock() {}
	void packetUnlock() {}
};

static TestPlatform g_TestPltf;
bool g_isInitialized = false;

class TraceCollectorTest : public ::testing::Test {
protected:
	void SetUp() override {
		if ( !g_isInitialized ) {
			auto *inst3 = traceCollector::getInstance();
			inst3->setStreamId( 0 );
			inst3->setPlatformIntf( &g_TestPltf );
			g_isInitialized = true;
		}
	}
};

// Test: Verify singleton instance
TEST_F( TraceCollectorTest, SingletonInstance ) {
	auto *inst1 = traceCollector::getInstance();
	auto *inst2 = traceCollector::getInstance();
	EXPECT_EQ( inst1, inst2 );

	// TODO: need to see what to do for UT
}

// Test: Push traces and send first packet
TEST_F( TraceCollectorTest, PushAndSendFirstPacket ) {
	auto *collector = traceCollector::getInstance();

	// Create mock traces
	auto evt			= new Trace<mock_trace_t>;
	mock_trace_t *param = evt->getParam();
	memset( param->value.data(), 0x11, 10 );

	// Push traces to fill current packet
	for ( int i = 0; i < CONFIG_TRACE_MAX_PER_PACKET; i++ ) {
		collector->pushTrace( evt );
	}

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
	auto *collector = traceCollector::getInstance();

	auto sendPacket = collector->getSendPacket();
	EXPECT_FALSE( sendPacket.has_value() );
}

// Test: Sequential packet sending with completion
TEST_F( TraceCollectorTest, SequentialPackets ) {
	auto *collector = traceCollector::getInstance();

	// Create mock traces
	auto evt1			 = new Trace<mock_trace_t>;
	mock_trace_t *param1 = evt1->getParam();
	memset( param1->value.data(), 0x11, 10 );

	// Push traces to fill current packet
	for ( int i = 0; i < CONFIG_TRACE_MAX_PER_PACKET; i++ ) {
		collector->pushTrace( evt1 );
	}

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
