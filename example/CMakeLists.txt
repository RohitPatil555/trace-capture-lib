# SPDX-License-Identifier: MIT | Author: Rohit Patil
# --------------------------------------------------------------------
# CMakeLists.txt
# --------------------------------------------------------------------
cmake_minimum_required(VERSION 3.25)
project(Example LANGUAGES CXX)

# ------------------------------------------------------------
# Global settings (C++20, sanitizers, coverage, compiler flags)
# ------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # useful for clangd / other tools

set(TRACE_DESCRIPTION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/example.yml)
set(TRACE_GENERATED_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

file(MAKE_DIRECTORY ${TRACE_GENERATED_OUT_DIR})

add_subdirectory(.. ${CMAKE_BINARY_DIR}/trace)

add_executable(example
        src/examplePlatform.cpp
        main.cpp
    )

target_include_directories(example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_link_libraries(example PRIVATE embdTraceLog)
add_dependencies(example generate_config)
