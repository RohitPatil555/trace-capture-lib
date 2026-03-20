# SPDX-License-Identifier: MIT | Author: Rohit Patil
# tests/CMakeLists.txt
enable_testing()

# Function: collect_gtest_names
# Usage:
#   collect_gtest_names(TEST_LIST_VAR file1.cpp file2.cpp ...)
# After calling, ${TEST_LIST_VAR} will contain a list like Suite::Test
function(collect_gtest_names OUT_VAR)
    set(result "")

    foreach(src IN LISTS ARGN)
        # Run awk on each file
        execute_process(
            COMMAND awk "/TEST(_F|_P)?\\(/ {
                            split($0, a, /[,|(|)]/);
                            gsub(/ /,\"\",a[3]);
                            print a[2] \".\" a[3];
                        }"
                        ${src}
            OUTPUT_VARIABLE TEST_NAMES_RAW
            OUTPUT_STRIP_TRAILING_WHITESPACE
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

        if(TEST_NAMES_RAW)
            string(REPLACE "\n" ";" TEST_NAMES "${TEST_NAMES_RAW}")
            list(APPEND result ${TEST_NAMES})
        endif()
    endforeach()

    list(REMOVE_DUPLICATES result)
    set(${OUT_VAR} "${result}" PARENT_SCOPE)
endfunction()

find_package(GTest REQUIRED)          # gtest/gmock via vcpkg, Conan or system

add_executable(tests main.cpp)
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../tmpl)
target_link_libraries(tests PRIVATE embdTraceLog GTest::gtest GTest::gmock)

# Add coverage if enabled
if(ENABLE_COVERAGE)
    target_compile_options(tests PRIVATE --coverage -O0)
endif()

if(SANITIZERS)
    target_link_options(tests PRIVATE ${SANITIZERS})
endif()

# Set additional sources for test
set(TESTS_SRCS basic.cpp
    packetPool.cpp
    packetOp.cpp
    traceCollectorTest.cpp
)

target_sources(tests PRIVATE ${TESTS_SRCS})
collect_gtest_names(TEST_LIST ${TESTS_SRCS})

message(STATUS "All collected tests: ${TEST_LIST}")
foreach(test_name ${TEST_LIST})
    string(STRIP "${test_name}" test_name_stripped)
    add_test(NAME ${test_name} COMMAND tests --gtest_filter=${test_name_stripped})
endforeach(test_name ${TEST_LIST})
