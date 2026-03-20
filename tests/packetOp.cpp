#include <gtest/gtest.h>

#include <internal/tracePacket.hpp>
#include <trace.hpp>

#define CONVERT_SIZE_IN_BITS( x ) ( ( x ) * 8 )

class MockTrace : public TraceIntf {
	std::vector<std::byte> m_data;

public:
	explicit MockTrace( size_t size, uint8_t fill_value = 0 ) : m_data( size ) {
		std::fill( m_data.begin(), m_data.end(), static_cast<std::byte>( fill_value ) );
	}

	void setTimestamp( uint64_t ts ) { (void)ts; }

	std::span<const std::byte> getTraceInRaw() override { return m_data; }
};

class TracePacketTest : public ::testing::Test {
protected:
	const uint32_t TEST_STREAM_ID			 = 0xABCD1234;
	const uint32_t TEST_SEQ_NO				 = 0x100;
	const uint32_t TEST_TRACE_MAX_SIZE		 = 64;
	const uint32_t TEST_TRACE_PADDING_SIZE_1 = 60;
	const uint32_t TEST_TRACE_DROP_COUNT	 = 10;
};

TEST_F( TracePacketTest, Initialization ) {
	tracePacket packet;
	const packet_buffer_t *pktBuf = nullptr;

	packet.init( TEST_STREAM_ID, TEST_SEQ_NO, 0ULL );
	packet.buildPacket( 0ULL );

	auto raw = packet.getPacketInRaw();
	pktBuf	 = reinterpret_cast<const packet_buffer_t *>( raw.data() );

	EXPECT_EQ( pktBuf->stream_id, TEST_STREAM_ID );
	EXPECT_EQ( pktBuf->traces_discarded, 0 );
	EXPECT_EQ( pktBuf->packet_seq_count, TEST_SEQ_NO );

	EXPECT_EQ( pktBuf->content_size, CONVERT_SIZE_IN_BITS( sizeof( uint32_t ) * 9 ) );
	EXPECT_EQ( pktBuf->packet_size, CONVERT_SIZE_IN_BITS( sizeof( packet_buffer_t ) ) );
}

TEST_F( TracePacketTest, CapacityManagement ) {
	tracePacket packet;
	const packet_buffer_t *pktBuf = nullptr;
	MockTrace mevt( TEST_TRACE_MAX_SIZE, 0x11 );

	packet.init( TEST_STREAM_ID, TEST_SEQ_NO, 0ULL );

	// Push trace till packet is full.
	while ( !packet.isPacketFull() ) {
		packet.addTrace( &mevt );
	}

	packet.buildPacket( 0ULL );

	auto raw = packet.getPacketInRaw();
	pktBuf	 = reinterpret_cast<const packet_buffer_t *>( raw.data() );

	EXPECT_EQ( pktBuf->stream_id, TEST_STREAM_ID );
	EXPECT_EQ( pktBuf->traces_discarded, 0 );
	EXPECT_EQ( pktBuf->packet_seq_count, TEST_SEQ_NO );

	EXPECT_EQ( pktBuf->content_size, CONVERT_SIZE_IN_BITS( sizeof( packet_buffer_t ) ) );
	EXPECT_EQ( pktBuf->packet_size, CONVERT_SIZE_IN_BITS( sizeof( packet_buffer_t ) ) );
}


TEST_F( TracePacketTest, PaddingValidation ) {
	tracePacket packet;
	const packet_buffer_t *pktBuf = nullptr;
	MockTrace mevt( TEST_TRACE_PADDING_SIZE_1, 0x22 );
	const uint32_t padSize =
		( TEST_TRACE_MAX_SIZE - TEST_TRACE_PADDING_SIZE_1 ) * CONFIG_TRACE_MAX_PER_PACKET;

	packet.init( TEST_STREAM_ID, TEST_SEQ_NO, 0ULL );

	// Push trace till packet is full.
	while ( !packet.isPacketFull() ) {
		packet.addTrace( &mevt );
	}

	packet.buildPacket( 0ULL );

	auto raw = packet.getPacketInRaw();
	pktBuf	 = reinterpret_cast<const packet_buffer_t *>( raw.data() );

	EXPECT_EQ( pktBuf->stream_id, TEST_STREAM_ID );
	EXPECT_EQ( pktBuf->traces_discarded, 0 );
	EXPECT_EQ( pktBuf->packet_seq_count, TEST_SEQ_NO );

	EXPECT_EQ( pktBuf->content_size, CONVERT_SIZE_IN_BITS( sizeof( packet_buffer_t ) - padSize ) );
	EXPECT_EQ( pktBuf->packet_size, CONVERT_SIZE_IN_BITS( sizeof( packet_buffer_t ) ) );
}

TEST_F( TracePacketTest, DropTraceValidation ) {
	tracePacket packet;
	const packet_buffer_t *pktBuf = nullptr;

	packet.init( TEST_STREAM_ID, TEST_SEQ_NO, 0ULL );

	for ( uint32_t i = 0; i < TEST_TRACE_DROP_COUNT; i++ ) {
		packet.dropTrace();
	}

	packet.buildPacket( 0ULL );

	auto raw = packet.getPacketInRaw();
	pktBuf	 = reinterpret_cast<const packet_buffer_t *>( raw.data() );

	EXPECT_EQ( pktBuf->stream_id, TEST_STREAM_ID );
	EXPECT_EQ( pktBuf->traces_discarded, TEST_TRACE_DROP_COUNT );
	EXPECT_EQ( pktBuf->packet_seq_count, TEST_SEQ_NO );

	EXPECT_EQ( pktBuf->content_size, CONVERT_SIZE_IN_BITS( sizeof( uint32_t ) * 9 ) );
	EXPECT_EQ( pktBuf->packet_size, CONVERT_SIZE_IN_BITS( sizeof( packet_buffer_t ) ) );
}
