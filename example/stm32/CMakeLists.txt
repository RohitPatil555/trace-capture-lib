# SPDX-License-Identifier: MIT | Author: Rohit Patil
# --------------------------------------------------------------------
# CMakeLists.txt
# --------------------------------------------------------------------
cmake_minimum_required(VERSION 3.25)
project(stm32 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Set to OFF for more portable code

set(CMAKE_CXX_FLAGS "${CPU_FLAGS} -fno-exceptions -fno-use-cxa-atexit -fno-rtti -fno-sized-deallocation -fno-threadsafe-statics -Wall -O0 -g")
set(CMAKE_C_FLAGS   "${CPU_FLAGS} -Wall -O0 -g")

set(TRACE_COLLECTION_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(TRACE_DESCRIPTION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/stm32_event.yml)
set(TRACE_GENERATED_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

file(MAKE_DIRECTORY ${TRACE_GENERATED_OUT_DIR})

add_subdirectory(${TRACE_COLLECTION_SOURCE_PATH} ${CMAKE_CURRENT_BINARY_DIR}/trace)

# Linker script
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)

add_executable(${PROJECT_NAME}.elf
    src/main.cpp
    src/scheduler.cpp
    src/tracePlatform.cpp
    hal/src/stm32/uart.cpp
)

target_sources(${PROJECT_NAME}.elf PRIVATE
    src/compiler_req_apis.c
)

target_include_directories(${PROJECT_NAME}.elf
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hal/core/inc
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hal/inc
        PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    -nostdlib
    -T${LINKER_SCRIPT}
)

target_link_libraries(${PROJECT_NAME}.elf PRIVATE embdTraceLog)
add_dependencies(${PROJECT_NAME}.elf generate_config)
