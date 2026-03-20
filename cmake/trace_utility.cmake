# SPDX-License-Identifier: MIT | Author: Rohit Patil
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(INPUT_YAML "${TRACE_DESCRIPTION_FILE}")

if (EXISTS "${TRACE_GENERATED_OUT_DIR}")
    set(OUTPUT_DIR "${TRACE_GENERATED_OUT_DIR}")
else ()
    set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/exported")
endif ()

set(TRACE_TYPE_HEADER "${OUTPUT_DIR}/trace_types.hpp")
set(TRACE_BABELTRACE_CONFIG "${OUTPUT_DIR}/metadata")

# Custom command to run the Python script
add_custom_command(
    OUTPUT "${TRACE_TYPE_HEADER}" "${TRACE_BABELTRACE_CONFIG}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_LIST_DIR}/../tools/trace_generate.py"
            "${INPUT_YAML}" "${OUTPUT_DIR}"
    DEPENDS "${INPUT_YAML}" "${CMAKE_CURRENT_LIST_DIR}/../tools/trace_generate.py"
    COMMENT "Running Python tool to generate header and config"
    VERBATIM
)

# Add a target that depends on generated files
add_custom_target(generate_config
    DEPENDS "${TRACE_TYPE_HEADER}" "${TRACE_BABELTRACE_CONFIG}"
)
