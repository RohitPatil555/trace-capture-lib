// SPDX-License-Identifier: MIT | Author: Rohit Patil

#pragma once

/* --------------------------------------------------------------------------
 *  Standard library headers
 * -------------------------------------------------------------------------- */
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

/* --------------------------------------------------------------------------
 *  Project headers
 * -------------------------------------------------------------------------- */
#include <config.hpp>
#include <trace.hpp>

/* Forward declaration – the packet type is defined elsewhere */
class tracePacket;

/* --------------------------------------------------------------------------
 *  Platform Interface
 *
 *  The `tracePlatform` class defines the only operations that the user must
 *  provide.  All methods are pure virtual so that any concrete platform can
 *  be plugged in without changing this header.
 * -------------------------------------------------------------------------- */
class tracePlatform {
public:
	/* Return a monotonically increasing timestamp (nanoseconds is common). */
	virtual uint64_t getTimestamp() = 0;

	/* Try an acquire an exclusive lock before mutating shared state.
	 * Return : True if success and False if fail to get within timeout */
	virtual bool traceTryLock() = 0;

	/* Release the lock after mutation is finished. */
	virtual void traceUnlock() = 0;
};

/* --------------------------------------------------------------------------
 *  Trace Collector
 *
 *  `traceCollector` is a singleton that collects traces into packets.
 *  It owns an opaque implementation object (`Impl`) stored in a fixed‑size
 *  buffer to avoid dynamic allocation on the critical path.  The collector
 *  keeps two packet pointers:
 *
 *      - currPkt : packet currently being built
 *      - sendPkt : packet that has been finished and is ready for sending
 * -------------------------------------------------------------------------- */
class traceCollector {
	std::span<tracePacket> tracePacketList;

	/* ----------------------------------------------------------------------
	 *  Platform pointer
	 *
	 *  The collector delegates timestamp and locking to this interface.
	 * ---------------------------------------------------------------------- */
	tracePlatform *pltf;

	/* ----------------------------------------------------------------------
	 *  Packet bookkeeping
	 *
	 *  Two packet pointers are maintained:
	 *      - currPkt: the packet being populated with traces
	 *      - sendPkt: the packet that is in progress to send out.
	 * ---------------------------------------------------------------------- */
	std::atomic<uint32_t> freeIndex;
	std::atomic<uint32_t> fullIndex;
	tracePacket *currPkt;

	uint32_t
		discardTraceCount; // number of traces dropped because the current packet is not available
	uint32_t pktSqnNo;	   // monotonically increasing sequence number for packets.
	uint32_t streamId;	   // identifier that will be embedded in each packet header.
	uint32_t packetMask = 0;

	/* lazily creates or re‑uses a packet for writing. */
	tracePacket *getCurrentPacket();

	/* finalises the current packet and hands it to send-Q */
	void updatePacketIndex();

	/* serialises an trace into the current packet.*/
	void sendTrace( TraceIntf *evt );

public:
	traceCollector( std::span<std::byte> buffer );

	static size_t getPacketBufferSize( uint32_t packetCount ) noexcept;

	/* ----------------------------------------------------------------------
	 *  Trace submission
	 *
	 *  The template accepts any type that satisfies the `IsTraceType`
	 *  concept.  It simply casts to the base interface and forwards it
	 *  to `sendTrace()`.
	 * ---------------------------------------------------------------------- */
	template <IsTraceType E> inline void pushTrace( E *ptr ) {
		TraceIntf *intf = ptr;
		sendTrace( intf );
	}

	/* ----------------------------------------------------------------------
	 *  Packet retrieval
	 *
	 *  If a packet has been queued for transmission, this returns a span over
	 *  its raw bytes.  The caller is responsible for handling the data and
	 *  then notifying the collector that sending is finished.
	 * ---------------------------------------------------------------------- */
	std::optional<std::span<const std::byte>> getSendPacket();
	void sendPacketCompleted(); // Notify that the platform has finished sending `sendPkt`

	/* --------------------------------------------------------------------
	 *  If system stuck and not generating enough trace to push packet for send.
	 *  In such scenario, call this API, this will force current packet to send
	 *  all collected trace.
	 *
	 * return : true mean packet sync succesfully else return false.
	 *          false indicate caller must retry after some time as current
	 * 				trace write operation going on.
	 * -------------------------------------------------------------------- */
	bool forceSync( void );

	/* ----------------------------------------------------------------------
	 *  Configuration helpers
	 *
	 *  streamId – sets the identifier that will be embedded in every packet.
	 *  pltf     – registers the platform interface implementation.
	 * ---------------------------------------------------------------------- */
	void setStreamId( uint32_t _streamId );
	void setPlatformIntf( tracePlatform *_pltf );
};
