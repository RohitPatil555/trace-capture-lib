// SPDX-License-Identifier: MIT | Author: Rohit Patil
/*********************************************************************
 *  traceCollector implementation
 *
 *  This file implements a lightweight runtime component that
 *  aggregates individual traces into packets for transmission.
 *  The design follows the P‑Impl idiom to keep the public interface
 *  stable while allowing internal changes without breaking ABI.
 *********************************************************************/

/* --------------------------------------------------------------------------
 *  Standard library headers
 * -------------------------------------------------------------------------- */
#include <cassert>

/* --------------------------------------------------------------------------
 *  Project headers
 * -------------------------------------------------------------------------- */
#include <internal/tracePacket.hpp>
#include <traceCollector.hpp>


using namespace std;

/* --------------------------------------------------------------------
 *  Constructor – initialise hidden implementation and state.
 *
 *  * `impl`   : placement‑new of Impl inside the storage buffer.
 * -------------------------------------------------------------------- */
traceCollector::traceCollector( std::span<std::byte> buffer ) {
	size_t maxPktCountInBuffer = 0;
	tracePacket *ptr		   = nullptr;

	discardTraceCount = 0;
	pktSqnNo		  = 0;
	streamId		  = 0;
	pltf			  = nullptr;

	maxPktCountInBuffer = ( buffer.size() / sizeof( tracePacket ) );
	assert( maxPktCountInBuffer >= 1 );
	assert( !( maxPktCountInBuffer & ( maxPktCountInBuffer - 1 ) ) );

	ptr				= reinterpret_cast<tracePacket *>( buffer.data() );
	tracePacketList = std::span<tracePacket>( ptr, maxPktCountInBuffer );

	freeIndex  = 0;
	fullIndex  = 0;
	currPkt	   = &tracePacketList[ 0 ];
	packetMask = maxPktCountInBuffer - 1;
}

/* --------------------------------------------------------------------
 *  Acquire the current packet for trace insertion.
 *
 *  If no packet is currently active, allocate one from the pool,
 *  initialise it with the current stream ID and sequence number
 *  and reset the discard counter.
 * -------------------------------------------------------------------- */
tracePacket *traceCollector::getCurrentPacket() {
	uint64_t ts				  = 0;
	uint32_t currentFreeIndex = 0;

	if ( currPkt == nullptr ) {
		currentFreeIndex = freeIndex.load( std::memory_order_relaxed );
		currPkt			 = &tracePacketList[ currentFreeIndex ];
		ts				 = pltf->getTimestamp();
		currPkt->init( streamId, pktSqnNo, ts );
		discardTraceCount = 0;
		pktSqnNo++;
	}

	return currPkt;
}

/* --------------------------------------------------------------------
 *  Send the current packet to the queue.
 *
 *  The packet must already exist (checked by assert).  It is
 *  inserted into the internal queue and then cleared so a new one
 *  can be built.  Because the queue capacity equals the pool size,
 *  insertion never fails – the following assert guarantees that.
 * -------------------------------------------------------------------- */
void traceCollector::updatePacketIndex() {
	uint32_t currFreeIndex = freeIndex.load( std::memory_order_relaxed );

	currPkt = nullptr;

	if ( ( ( currFreeIndex + 1 ) & packetMask ) == fullIndex.load( std::memory_order_acquire ) ) {
		// packet queue is full
		return;
	}

	// move to next index.
	currFreeIndex = ( currFreeIndex + 1 ) & packetMask;
	freeIndex.store( currFreeIndex, std::memory_order_release );
}

size_t traceCollector::getPacketBufferSize( uint32_t packetCount ) noexcept {
	assert( ( packetCount & ( packetCount - 1 ) ) == 0 );
	return ( sizeof( tracePacket ) * packetCount );
}

/* --------------------------------------------------------------------
 *  Public API – add an trace to the collector.
 *
 *  1. Obtain or create a current packet.
 *  2. Acquire platform timestamp and store it in the trace.
 *  3. Add the trace to the packet (thread‑safe via platform lock).
 *  4. If the packet becomes full, build its wire format and
 *     enqueue it for sending.
 * -------------------------------------------------------------------- */
void traceCollector::sendTrace( TraceIntf *evt ) {
	tracePacket *curr = nullptr;
	uint64_t _ts	  = 0;

	if ( !pltf->traceTryLock() ) {
		curr->dropTrace();
		return;
	}

	curr = getCurrentPacket();

	// Queue is not free and packet is also full keep droping packet.
	if ( curr->isPacketFull() ) {
		discardTraceCount++;
		// pool for free packet.
		updatePacketIndex();

		pltf->traceUnlock(); // free lock and return.
		return;
	}

	// Add event to packet.
	_ts = pltf->getTimestamp();
	evt->setTimestamp( _ts );
	curr->addTrace( evt );

	// Check packet is full then build payload.
	if ( curr->isPacketFull() ) {
		_ts = pltf->getTimestamp();
		curr->buildPacket( _ts );
		updatePacketIndex();
	}

	pltf->traceUnlock();
}

/* --------------------------------------------------------------------
 *  Callback invoked when a previously sent packet has been processed.
 *
 *  The packet is returned to the pool so it can be reused.
 * -------------------------------------------------------------------- */
void traceCollector::sendPacketCompleted() {
	uint32_t currFullIndex = 0;

	currFullIndex = fullIndex.load( std::memory_order_relaxed );
	if ( currFullIndex == freeIndex.load( std::memory_order_acquire ) ) {
		return;
	}

	currFullIndex = ( currFullIndex + 1 ) & packetMask;
	fullIndex.store( currFullIndex, std::memory_order_release );
}

/* --------------------------------------------------------------------
 *  If system stuck and not generating enough trace to push packet for send.
 *  In such scenario, call this API, this will force current packet to send
 *  all collected trace.
 * -------------------------------------------------------------------- */
bool traceCollector::forceSync( void ) {
	if ( currPkt == nullptr ) {
		// No packet to flush hence return success.
		// this indicate packet in send list need to be pull by sender routine
		return true;
	}

	if ( !pltf->traceTryLock() ) {
		return false;
	}

	uint64_t _ts	  = 0;
	tracePacket *curr = getCurrentPacket();

	_ts = pltf->getTimestamp();
	curr->buildPacket( _ts );

	updatePacketIndex();

	pltf->traceUnlock();
	return true;
}

/* --------------------------------------------------------------------
 *  Retrieve a ready‑to‑send packet for transmission.
 *
 *  If no packet is currently cached, pull one from the queue.  The
 *  caller receives an optional byte span that points to the raw
 *  packet buffer; if the queue was empty `std::nullopt` is returned.
 * -------------------------------------------------------------------- */
std::optional<std::span<const std::byte>> traceCollector::getSendPacket() {
	tracePacket *sendPacketPtr = nullptr;
	uint32_t currFullIndex	   = 0;

	currFullIndex = fullIndex.load( std::memory_order_relaxed );

	if ( currFullIndex == freeIndex.load( std::memory_order_acquire ) ) {
		// list is empty
		return nullopt;
	}

	sendPacketPtr = &tracePacketList[ currFullIndex ];

	return std::optional<std::span<const std::byte>>( sendPacketPtr->getPacketInRaw() );
}

/* --------------------------------------------------------------------
 *  Configure the stream identifier for packets.
 *
 *  The call must be idempotent – it is only legal to set the ID
 *  once during initialization.  An assertion guards against misuse.
 * -------------------------------------------------------------------- */
void traceCollector::setStreamId( uint32_t _streamId ) {
	// either called 2 time or some error in init path.
	assert( streamId == 0 );

	streamId = _streamId;
}

/* --------------------------------------------------------------------
 *  Attach the platform interface implementation.
 *
 *  The collector uses this to obtain timestamps and perform
 *  thread‑synchronisation.  It must only be set once; otherwise
 *  a double‑initialisation would corrupt state.
 * -------------------------------------------------------------------- */
void traceCollector::setPlatformIntf( tracePlatform *_pltf ) {
	assert( pltf == nullptr );

	pltf = _pltf;
}
