# SPDX-License-Identifier: MIT | Author: Rohit Patil
# src/CMakeLists.txt
add_library(embdTraceLog STATIC)

target_include_directories(embdTraceLog PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../genHdr>
)

target_include_directories(embdTraceLog PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../tmpl>
    $<INSTALL_INTERFACE:include>
)

# Link with sanitizers if needed (they are added globally already)
if(SANITIZERS)
    target_link_options(embdTraceLog PRIVATE ${SANITIZERS})
endif()

set(EMBD_TRACE_LOG_SORUCES
        traceCollector.cpp
        tracePacket.cpp
)

target_sources(embdTraceLog PRIVATE ${EMBD_TRACE_LOG_SORUCES})

# Export the library for other projects
export(TARGETS embdTraceLog FILE EmbdTraceLogConfig.cmake)
