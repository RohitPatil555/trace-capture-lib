// SPDX-License-Identifier: MIT | Author: Rohit Patil

#pragma once

/* --------------------------------------------------------------------------
 *  Standard library headers
 * -------------------------------------------------------------------------- */
#include <array>
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

	/* Acquire an exclusive lock for packet collection and send */
	virtual void packetLock() = 0;

	/* Release the lock for packet after mutation is finished. */
	virtual void packetUnlock() = 0;
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
	/* ----------------------------------------------------------------------
	 *  Impl
	 *
	 *  The actual implementation details are hidden behind the pointer `impl`.
	 *  To keep memory usage predictable, the implementation is stored inside
	 *  a fixed‑size array (`storage`) that is large enough for any expected
	 *  layout.  The size of this buffer is intentionally small – it can be
	 *  tuned by changing `ImplSize` if the implementation grows.
	 * ---------------------------------------------------------------------- */
	struct Impl;						   // forward declaration
	static constexpr size_t ImplSize = 48; // bytes reserved for Impl
	alignas( std::max_align_t ) std::array<std::byte, ImplSize> storage;

	Impl *impl;

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
	tracePacket *currPkt;
	tracePacket *sendPkt;

	uint32_t
		discardTraceCount; // number of traces dropped because the current packet is not available
	uint32_t pktSqnNo;	   // monotonically increasing sequence number for packets.
	uint32_t streamId;	   // identifier that will be embedded in each packet header.

	/* ----------------------------------------------------------------------
	 *  Private constructor
	 *
	 *  The singleton is created on first use via `getInstance()`.  The ctor
	 *  sets up the Impl object inside the storage buffer.
	 * ---------------------------------------------------------------------- */
	traceCollector();

	/* lazily creates or re‑uses a packet for writing. */
	tracePacket *getCurrentPacket();

	/* finalises the current packet and hands it to send-Q */
	void sendPacket();

	/* serialises an trace into the current packet.*/
	void sendTrace( TraceIntf *evt );

public:
	/* ----------------------------------------------------------------------
	 *  Singleton accessor
	 *
	 *  `noexcept` guarantees that retrieving the instance cannot throw.
	 * ---------------------------------------------------------------------- */
	static traceCollector *getInstance() noexcept;

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
	 * -------------------------------------------------------------------- */
	void forceSync( void );

	/* ----------------------------------------------------------------------
	 *  Configuration helpers
	 *
	 *  streamId – sets the identifier that will be embedded in every packet.
	 *  pltf     – registers the platform interface implementation.
	 * ---------------------------------------------------------------------- */
	void setStreamId( uint32_t _streamId );
	void setPlatformIntf( tracePlatform *_pltf );
};
