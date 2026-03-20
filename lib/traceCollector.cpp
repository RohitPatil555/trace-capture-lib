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
#include <Queue.hpp>
#include <internal/tracePacket.hpp>
#include <staticPool.hpp>
#include <traceCollector.hpp>

/* --------------------------------------------------------------------
 *  Global pool of packet objects.
 *
 *  The pool size is defined by the configuration macro
 *  CONFIG_PACKET_COUNT_MAX.  It provides fast allocation and
 *  deallocation without dynamic memory fragmentation.
 * -------------------------------------------------------------------- */
static StaticPool<tracePacket, CONFIG_PACKET_COUNT_MAX> pktPool;

/* --------------------------------------------------------------------
 *  Private implementation of traceCollector (P‑Impl).
 *
 *  The Impl only contains a queue that holds pointers to packets
 *  ready for transmission.  The queue capacity matches the pool
 *  size so no allocation failure can occur.
 * -------------------------------------------------------------------- */
struct traceCollector::Impl {
	Queue<tracePacket_ptr_t, CONFIG_PACKET_COUNT_MAX> queue;
};

/* --------------------------------------------------------------------
 *  Constructor – initialise hidden implementation and state.
 *
 *  * `impl`   : placement‑new of Impl inside the storage buffer.
 * -------------------------------------------------------------------- */
traceCollector::traceCollector() {
	static_assert( ImplSize == sizeof( Impl ), "hidden implementation size not matching" );

	impl			  = new ( storage.data() ) Impl();
	sendPkt			  = nullptr;
	currPkt			  = nullptr;
	discardTraceCount = 0;
	pktSqnNo		  = 0;
	streamId		  = 0;
	pltf			  = nullptr;
}

/* --------------------------------------------------------------------
 *  Acquire the current packet for trace insertion.
 *
 *  If no packet is currently active, allocate one from the pool,
 *  initialise it with the current stream ID and sequence number
 *  and reset the discard counter.
 * -------------------------------------------------------------------- */
tracePacket *traceCollector::getCurrentPacket() {
	uint64_t ts = 0;

	if ( currPkt == nullptr ) {
		pltf->packetLock();
		currPkt = pktPool.allocate();
		pltf->packetUnlock();

		ts = pltf->getTimestamp();
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
void traceCollector::sendPacket() {
	bool qstatus = false;
	// this must not be null in this path as per design.
	assert( currPkt != nullptr );

	pltf->packetLock();
	qstatus = impl->queue.insert( currPkt );

	// As queue size and packet buffer have same count it
	// never get asserted.
	assert( qstatus );

	currPkt = nullptr;
	pltf->packetUnlock();
}

/* --------------------------------------------------------------------
 *  Singleton accessor.
 *
 *  The collector is a global, stateless object.  This function
 *  returns the single instance.
 * -------------------------------------------------------------------- */
traceCollector *traceCollector::getInstance() noexcept {
	static traceCollector traceInst;

	return &traceInst;
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
	tracePacket *curr = getCurrentPacket();
	uint64_t _ts	  = 0;

	if ( curr == nullptr ) {
		discardTraceCount++;
		return;
	}

	if ( !pltf->traceTryLock() ) {
		curr->dropTrace();
		return;
	}

	_ts = pltf->getTimestamp();
	evt->setTimestamp( _ts );
	curr->addTrace( evt );
	pltf->traceUnlock();

	if ( curr->isPacketFull() ) {
		_ts = pltf->getTimestamp();
		curr->buildPacket( _ts );
		sendPacket();
	}
}

/* --------------------------------------------------------------------
 *  Callback invoked when a previously sent packet has been processed.
 *
 *  The packet is returned to the pool so it can be reused.
 * -------------------------------------------------------------------- */
void traceCollector::sendPacketCompleted() {
	if ( sendPkt != nullptr ) {
		pltf->packetLock();
		pktPool.release( sendPkt );
		pltf->packetUnlock();

		sendPkt = nullptr;
	}
}

/* --------------------------------------------------------------------
 *  If system stuck and not generating enough trace to push packet for send.
 *  In such scenario, call this API, this will force current packet to send
 *  all collected trace.
 * -------------------------------------------------------------------- */
void traceCollector::forceSync( void ) {
	if ( currPkt == nullptr ) {
		// No packet to flush hence return early.
		return;
	}

	uint64_t _ts	  = 0;
	tracePacket *curr = getCurrentPacket();

	_ts = pltf->getTimestamp();
	curr->buildPacket( _ts );

	sendPacket();
}

/* --------------------------------------------------------------------
 *  Retrieve a ready‑to‑send packet for transmission.
 *
 *  If no packet is currently cached, pull one from the queue.  The
 *  caller receives an optional byte span that points to the raw
 *  packet buffer; if the queue was empty `std::nullopt` is returned.
 * -------------------------------------------------------------------- */
std::optional<std::span<const std::byte>> traceCollector::getSendPacket() {
	if ( sendPkt == nullptr ) {
		pltf->packetLock();

		auto pkt = impl->queue.remove();

		pltf->packetUnlock();

		if ( !pkt.has_value() ) {
			return std::nullopt;
		}
		sendPkt = pkt.value();
	}

	return std::optional<std::span<const std::byte>>( sendPkt->getPacketInRaw() );
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
