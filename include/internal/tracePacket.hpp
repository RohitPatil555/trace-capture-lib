// SPDX-License-Identifier: MIT | Author: Rohit Patil

#pragma once

/* --------------------------------------------------------------------------
 *  Standard library headers
 * -------------------------------------------------------------------------- */
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

/* --------------------------------------------------------------------------
 *  Project headers
 * -------------------------------------------------------------------------- */
#include <config.hpp>
#include <trace.hpp>

/* --------------------------------------------------------------------------
 *  Raw packet buffer layout
 *
 *  The struct is marked `packed` so that the memory representation matches
 *  what will be transmitted over the wire (no padding bytes).
 * -------------------------------------------------------------------------- */
typedef struct {
	uint32_t stream_id;		   // ID of the originating stream
	uint64_t timestamp_begin;  // Begining timestamp
	uint64_t timestamp_end;	   // End timestamp
	uint32_t traces_discarded; // how many traces were dropped before adding to this packet
	uint32_t packet_size;	   // total size of the packet in bytes (header + payload)
	uint32_t content_size;	   // size of the trace payload only
	uint32_t packet_seq_count; // sequence number for ordering packets

	/* Fixed‑size buffer that will hold the concatenated raw bytes of all traces. */
	std::array<uint8_t, TRACE_MAX_PAYLOAD_IN_BYTES> tracePayload;
} __attribute__( ( packed ) ) packet_buffer_t;

/* --------------------------------------------------------------------------
 *  TracePacket – a small helper class that builds a packet from Traces
 *
 *  It does not expose any public data members; everything is encapsulated.
 * -------------------------------------------------------------------------- */
class tracePacket {
	/* Current offset inside the payload array where the next trace will be copied. */
	std::size_t currOffset;

	/* Number of traces already added to this packet. */
	std::size_t traceCount;

	/* The raw memory buffer that represents the packet. */
	packet_buffer_t buffer;

public:
	tracePacket() = default;
	~tracePacket();

	/* Initialise a new packet with a stream identifier and a sequence number. */
	void init( uint32_t streamId, uint32_t seqNo, uint64_t ts );

	/* Return true if adding another trace would overflow the payload array. */
	bool isPacketFull();

	/* Add an Trace to the packet; returns false if the packet is already full. */
	bool addTrace( TraceIntf *tracePtr );

	/* Increment the counter of dropped traces (used when a packet overflows). */
	void dropTrace() { buffer.traces_discarded++; }

	/* Return a read‑only byte span that contains the entire packet (header + payload). */
	std::span<const std::byte> getPacketInRaw();

	/* Finalise the packet: compute sizes, set sequence numbers, etc. */
	void buildPacket( uint64_t ts );
};

/* --------------------------------------------------------------------------
 *  Convenience typedef for an tracePacket pointer
 * -------------------------------------------------------------------------- */
typedef tracePacket *tracePacket_ptr_t;
