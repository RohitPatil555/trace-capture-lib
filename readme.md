# Trace Capture Library (CTF 1.8)

A lightweight C++ library that lets you capture traces on bare‑metal or embedded systems and emit them in the **CTF 1.8** format – ready for consumption by tools such as `babeltrace2`.
The library is intentionally minimal so it can be dropped into any RTOS, bootloader, or no‑OS project.

> *Why CTF?*
> The Linux `perf` subsystem and many modern trace analyzers rely on the CTF format. By emitting CTF from your firmware you get instant access to a rich ecosystem of visualizers, filters, and exporters without having to write custom parsers.

---

## Table of Contents

- [Introduction](#introduction)
- [Features](#features)
- [Quick Start](#quick-start)
  - [1. Create an trace description YAML](#1-create-an-trace-description-yaml)
  - [2. Generate the header with CMake](#2-generate-the-header-with-cmake)
  - [3. Integrate into your code](#3-integrate-into-your-code)
- [API Reference](#api-reference)
  - [`traceCollector`](#tracecollector)
  - `Trace<T>` – typed trace wrapper
  - Platform abstraction (`tracePlatform`)
- [Transferring Packets](#transferring-packets)
- [Processing & Analysis](#processing--analysis)
- [Advanced Usage](#advanced-usage)
  - Custom synchronization
  - Multi‑stream support
- [FAQ / Troubleshooting](#faq--troubleshooting)
- [License](#license)

---

## Introduction

Embedded and bare‑metal firmware is getting increasingly complex.
To debug performance issues, race conditions or to understand system behaviour you often need a trace collector that:

* Works without an OS.
* Emits data in a standard format.
* Is easy to drop into any codebase.

This library fulfills those needs by:

| Feature | Description |
|---------|-------------|
| **CTF 1.8 output** | Portable binary traces consumable by `babeltrace2`, `tracing‑tools`, and custom parsers. |
| **Zero runtime cost** | All trace types are generated at compile time – no dynamic allocation. |
| **Header‑only API** | No external dependencies beyond the standard library and CMake. |
| **YAML configuration** | Define traces, parameters, IDs once in a human‑readable file. |

---

## Quick Start

### 1. Create an trace description YAML

```yaml
# traces.yaml
- traces:
  - name: loopCount
    id: 1
    params:
      - name: count
        type: uint8_t
```

> **Tip** – Keep the file in your project root or a dedicated `docs/` folder.

### 2. Generate the header with CMake

Add the following to your top‑level `CMakeLists.txt`:

```cmake
# Path to the YAML description
set(TRACE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/traces.yaml")

# Where generated files should go
set(TRACE_GENERATED_OUT_DIR "${CMAKE_BINARY_DIR}/generated")

add_subdirectory(path/to/trace-capture-lib)

# Expose the header for your application
target_include_directories(my_app PRIVATE ${TRACE_GENERATED_OUT_DIR})
```

When you run CMake, it will:

1. Parse `traces.yaml`.
2. Generate `trace_types.hpp` containing typed structs (e.g., `loopCount_t`) and helper macros.
3. Create a metadata file (`metadata.json`) for babeltrace.

### 3. Integrate into your code

```cpp
#include "trace_types.hpp"          // generated header
#include "trace_collector.h"

int main() {
    /* ---------- Init Collector ---------- */
    auto& tc = traceCollector::getInstance();

    // Provide a platform implementation (see below)
    extern tracePlatform g_platform;   // defined elsewhere
    tc.setStreamId(42);                // any uint32_t stream ID
    tc.setPlatformIntf(&g_platform);

    /* ---------- Emit an trace ---------- */
    Trace<loopCount_t> evt;
    auto* p = evt.getParam();          // get the underlying struct
    p->count = 7;                      // fill parameters

    tc.pushTrace(&evt);                // send to collector

    /* ---------- Transfer packet ---------- */
    auto pkt = tc.getSendPacket();
	if ( pkt.has_value() ) {
		auto data = pkt.value();

        // e.g., write to UART, file or socket
		my_transmit( reinterpret_cast<const char *>( data.data() ), data.size() );

		tc.sendPacketCompleted();
	}

    return 0;
}
```

---

## API Reference

### `traceCollector`

*Singleton pattern.*

| Method | Description |
|--------|-------------|
| `static traceCollector& getInstance()` | Retrieve the global instance. |
| `void setStreamId(uint32_t id)` | Set CTF stream ID (must be unique per trace). |
| `void setPlatformIntf(tracePlatform* plt)` | Provide platform‑specific timestamp & lock functions. |
| `bool pushTrace(const traceBase* ev)` | Queue an trace for the next packet. |
| `const uint8_t* getSendPacket()` | Get pointer to ready‑to‑send CTF packet. |
| `size_t getPacketLength()` | Length of the packet in bytes. |
| `void sendPacketCompleted()` | Release buffer; collector can reuse it. |

### `Trace<T>`

A lightweight wrapper around a generated trace type.

```cpp
template<typename T>
class Trace {
public:
    // Access underlying CTF struct
    T* getParam();

    // Optional: access metadata, timestamp, etc.
};
```

The actual data structure is defined in `trace_types.hpp`, e.g.:

```cpp
struct loopCount_t {
    uint8_t count;
};
```

### Platform abstraction – `tracePlatform`

You must provide an implementation that the collector uses for:

| Function | Purpose |
|----------|---------|
| `uint64_t getTimestamp()` | Return monotonic timestamp (e.g., from a high‑resolution timer). |
| `bool traceTryLock()` | Acquire exclusive lock while emitting traces. |
| `void traceUnlock()` | Release the lock. |
| `void packetLock()` | Acquire exclusive lock while emitting traces. |
| `void packetUnlock()` | Release the lock. |

```cpp
class MyPlatform : public tracePlatform {
public:
    uint64_t getTimestamp() override { return hw_get_time(); }
    bool traceTryLock()   override { return critical_try_section_enter(); }
    void traceUnlock() override { critical_section_exit();  }
    void packetLock()   override { critical_section_enter(); }
    void packetUnlock() override { critical_section_exit();  }
};
```

Instantiate it once and register with the collector.

---

## Transferring Packets

The collector batches traces into CTF packets.
Typical flow:

```cpp
auto pkt = inst->getSendPacket();
auto data = pkt.value();

// Example: write to file
std::ofstream out("trace.bin", std::ios::binary | std::ios::app);
out.write(reinterpret_cast<const char*>(data.data()), data.size());

// Or send over UART / network...
```

After the packet is transmitted, call:

```cpp
tc.sendPacketCompleted();
```

This signals that the buffer can be reused for subsequent traces.

---

## Processing & Analysis

1. **Collect packets** – Store them in a single binary file or separate `_X.bin` files.
2. **Place metadata** – The generator creates `metadata.json`. Keep it next to your data:

```
traces/
├─ metadata.json
└─ stream.bin      # concatenated packet stream
```

3. **Run Babeltrace**:

```bash
babeltrace -f binary traces/stream.bin > trace.txt
# or view in the GUI:
babeltrace --gui traces/stream.bin
```

You can also write custom Python scripts using `babeltrace2`’s Python bindings to filter or aggregate traces.

---

## Advanced Usage

### Custom Synchronization

If your platform does not provide a global lock, you can skip it by providing no‑op functions.
Just make sure the collector is thread‑safe for your use case.

```cpp
void traceTryLock()   override { /* nothing */ }
void traceUnlock() override { /* nothing */ }
```

### Multi‑Stream Support

The library supports multiple CTF streams per trace.
Create separate `traceCollector` instances (or re‑use the singleton with different IDs) and tag each packet accordingly.

---

## FAQ / Troubleshooting

| Question | Answer |
|----------|--------|
| **Where does the generated header go?** | In `${TRACE_GENERATED_OUT_DIR}` – you must add this to your include path. |
| **Why are my packets empty?** | Ensure `setStreamId()` and `setPlatformIntf()` are called before any `pushTrace()`. |
| **How large can a packet be?** | The collector preallocates a buffer (default 1 MiB). Increase with `TRACE_PACKET_SIZE` CMake variable if needed. |
| **Can I use this on an RTOS?** | Yes – just provide the platform callbacks. Avoid dynamic memory allocation; the library is header‑only. |
| **How do I add new trace types?** | Edit `traces.yaml`, rerun CMake, and rebuild. |

---

## License

MIT © 2025 Rohit Patil

---
