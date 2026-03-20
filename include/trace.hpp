// SPDX-License-Identifier: MIT | Author: Rohit Patil

#pragma once

/* --------------------------------------------------------------------------
 *  Standard library headers
 * -------------------------------------------------------------------------- */
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

/* --------------------------------------------------------------------------
 *  Project headers
 * -------------------------------------------------------------------------- */
#include <config.hpp>

/* --------------------------------------------------------------------------
 *  Concept: TraceMemCopyable
 *
 *  A type T is considered an TraceMemCopyable if it satisfies the following:
 *      • Standard layout (no virtual functions, consistent memory layout)
 *      • Trivially copyable (can be mem‑copied safely)
 *      • Aggregate (plain struct/array with no constructors)
 *      • Non‑polymorphic
 *      • Size does not exceed CONFIG_TRACE_SIZE_MAX
 *
 *  These constraints guarantee that the trace can be stored and transmitted as a raw byte buffer.
 * -------------------------------------------------------------------------- */
template <typename T>
concept TraceMemCopyable =
	std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T> && std::is_aggregate_v<T> &&
	!std::is_polymorphic_v<T> && sizeof( T ) <= CONFIG_TRACE_SIZE_MAX;

/* Forward declaration of TraceId.
 *   TraceId<T>::value must provide a unique 32‑bit identifier for the payload type T. */
template <typename T> struct TraceId;

/* --------------------------------------------------------------------------
 *  Abstract interface for all traces
 *
 *  All concrete trace classes derive from this and must implement:
 *      • getTraceInRaw() – returns a byte view of the entire trace object.
 *      • setTimestamp(uint64_t) – records when the trace was generated/received.
 * -------------------------------------------------------------------------- */
class TraceIntf {
public:
	TraceIntf()			 = default;
	virtual ~TraceIntf() = default;

	virtual std::span<const std::byte> getTraceInRaw() = 0;
	virtual void setTimestamp( uint64_t ts )		   = 0;
};

/* --------------------------------------------------------------------------
 *  Concrete Trace implementation
 *
 *  T must satisfy TraceMemCopyable.
 *  The payload structure is packed to avoid any compiler‑added padding.
 * -------------------------------------------------------------------------- */
template <TraceMemCopyable T> class Trace final : public TraceIntf {
	/* Packed trace payload that will be serialised as raw bytes. */
	struct __attribute__( ( packed ) ) TracePayload {
		uint32_t id;
		uint64_t timestamp;
		T param;
	} evtPayload;

public:
	/* Constructor initialises the id field from TraceId<T> */
	Trace() { evtPayload.id = TraceId<T>::value; }
	~Trace() = default;

	/* Return a pointer to the payload so callers can read/write it. */
	T *getParam() { return &evtPayload.param; }

	/* Implement the interface: expose the whole trace as a byte span. */
	std::span<const std::byte> getTraceInRaw() {
		return std::as_bytes( std::span<TracePayload>( &evtPayload, 1 ) );
	}

	/* Store the timestamp in the packed payload. */
	void setTimestamp( uint64_t ts ) { evtPayload.timestamp = ts; }
};

/* --------------------------------------------------------------------------
 *  Type trait and concept to recognise trace types
 *
 *  is_trace_t<T> is true if T is an instantiation of Trace<…>.
 *  IsTraceType<U> can be used in generic code to constrain templates.
 * -------------------------------------------------------------------------- */
template <typename U> struct is_trace_t : std::false_type {};
template <typename T> struct is_trace_t<Trace<T>> : std::true_type {};
template <typename U> inline constexpr bool is_trace_v = is_trace_t<U>::value;

template <typename U>
concept IsTraceType = is_trace_v<U>;
