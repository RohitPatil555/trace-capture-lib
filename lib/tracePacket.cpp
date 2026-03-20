
/* --------------------------------------------------------------------------
 *  Standard library headers
 * -------------------------------------------------------------------------- */
#include <cstring>

/* --------------------------------------------------------------------------
 *  Project headers
 * -------------------------------------------------------------------------- */
#include <internal/tracePacket.hpp>

using namespace std;
/* --------------------------------------------------------------------
 * Destructor: Zero out the packet buffer when an tracePacket is destroyed.
 * This helps avoid leaking sensitive information or leaving stale data in memory.
 * -------------------------------------------------------------------- */
tracePacket::~tracePacket() { memset( &buffer, 0, sizeof( buffer ) ); }

/* --------------------------------------------------------------------
 * Initialise a new packet with a stream identifier and sequence number.
 * The internal offset counters are reset and the header fields are
 * populated.  The payload area is cleared to ensure no leftover data
 * from a previous use contaminates the new packet.
 * -------------------------------------------------------------------- */
void tracePacket::init( uint32_t streamId, uint32_t seqNo, uint64_t ts ) {
	currOffset = 0;
	traceCount = 0;

	memset( &buffer, 0, sizeof( buffer ) );
	buffer.stream_id		= streamId;
	buffer.packet_seq_count = seqNo;
	buffer.timestamp_begin	= ts;
}

/* --------------------------------------------------------------------
 * Check whether the packet has reached its maximum number of traces.
 * Returns true when no more traces can be added; otherwise false.
 * -------------------------------------------------------------------- */
bool tracePacket::isPacketFull() {
	if ( traceCount < CONFIG_TRACE_MAX_PER_PACKET ) {
		return false;
	}

	return true;
}

/* --------------------------------------------------------------------
 * Append a single trace to the packet payload.
 * The caller must have verified that the packet is not full.
 * Copies the raw byte representation of the trace into the buffer
 * and updates the offset/size counters accordingly.
 * Returns true on success, false if the packet was already full.
 * -------------------------------------------------------------------- */
bool tracePacket::addTrace( TraceIntf *tracePtr ) {
	span<const byte> tracePayload;

	// Prtrace overflow: do not add when capacity is exhausted.
	if ( isPacketFull() ) {
		return false;
	}

	// Retrieve the raw, byte‑wise representation of the trace.
	tracePayload = tracePtr->getTraceInRaw();

	// Copy the payload into the packet buffer at the current offset.
	memcpy( &buffer.tracePayload[ currOffset ], tracePayload.data(), tracePayload.size() );

	// Update bookkeeping values for next insertion.
	currOffset += tracePayload.size();
	traceCount++;

	return true;
}

/* --------------------------------------------------------------------
 * Finalise the packet by computing its size fields.
 * The packet header is updated with total packet size (in bits)
 * and the content size (header + payload, in bits).
 * -------------------------------------------------------------------- */

void tracePacket::buildPacket( uint64_t ts ) {
	size_t hdrSize = 0;

	hdrSize				 = sizeof( buffer ) - buffer.tracePayload.size();
	buffer.packet_size	 = sizeof( buffer ) * 8;		 // convert to bit
	buffer.content_size	 = ( hdrSize + currOffset ) * 8; // convert to bit
	buffer.timestamp_end = ts;
}

/* --------------------------------------------------------------------
 * Return the entire packet as a byte span.
 * The caller can then transmit or otherwise process the raw data.
 * -------------------------------------------------------------------- */
span<const byte> tracePacket::getPacketInRaw() { return as_bytes( span( &buffer, 1 ) ); }
