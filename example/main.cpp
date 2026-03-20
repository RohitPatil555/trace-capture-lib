// SPDX-License-Identifier: MIT | Author: Rohit Patil

#include <fstream>
#include <iostream>
#include <thread>

#include <examplePlatform.hpp>

using namespace std;

/*
 * Example program showing how to use the trace‑collector API.
 *
 * The collector is platform‑dependent; therefore we instantiate a
 * TestPlatform object here and register it with the collector before
 * any traces are pushed.
 */
static TestPlatform g_pltf;

/*
 * Posts a series of traces in a tight loop.
 *
 * Each trace carries an incrementing counter (type `loopCount_t`).
 * The function can be used to stress‑test timing or performance
 * characteristics of the collector.
 */
void trace_loop_index( uint32_t maxLoopCount ) {
	uint32_t idx = 0;
	Trace<loopCount_t> loopIdx;
	Trace<loopTime_t> loopTime;
	loopCount_t *param	  = nullptr;
	loopTime_t *timeParam = nullptr;
	traceCollector *inst  = nullptr;

	/* Retrieve the singleton instance of the collector. */
	inst = traceCollector::getInstance();

	timeParam = loopTime.getParam();
	param	  = loopIdx.getParam();

	timeParam->state = 1;
	inst->pushTrace( &loopTime );

	for ( idx = 0; idx < maxLoopCount; idx++ ) {
		/* Update the counter and push the trace. */
		param->count = idx + 1;
		inst->pushTrace( &loopIdx );
	}

	timeParam->state = 0;
	inst->pushTrace( &loopTime );
}

/*
 * Dumps all packets collected so far into a binary file.
 *
 * The function forces the collector to flush its buffers, then
 * repeatedly retrieves packets via `getSendPacket()`.  Each packet
 * is written to the supplied output stream and marked as sent
 * using `sendPacketCompleted()`.
 */
bool dumpFile( string_view filePath ) {
	ofstream ofs( filePath.data(), ios::binary );
	traceCollector *inst = nullptr;

	if ( !ofs ) {
		cerr << "Failed to open file.\n";
		return false;
	}

	/* Retrieve the singleton instance of the collector. */
	inst = traceCollector::getInstance();

	/* Flush pending data so that packets are available for export. */
	inst->forceSync();
	auto pkt = inst->getSendPacket();

	/* Write all available packets to the file. */
	while ( pkt.has_value() ) {
		auto data = pkt.value();
		ofs.write( reinterpret_cast<const char *>( data.data() ), data.size() );
		inst->sendPacketCompleted(); // Mark packet as transmitted.
		pkt = inst->getSendPacket(); // Get next packet, if any.
	}

	ofs.close();

	return true;
}

int main() {
	traceCollector *inst = nullptr;

	/* Initialise the collector with a stream ID and platform interface. */
	inst = traceCollector::getInstance();
	inst->setStreamId( TRACE_STREAM_ID );
	inst->setPlatformIntf( &g_pltf );

	/* Generate sample traces. */
	trace_loop_index( 10 );

	/* Export the collected data to a file. */
	if ( !dumpFile( "stream.bin" ) ) {
		cerr << "Stream is not captured " << endl;
		return -1;
	}

	return 0;
}
