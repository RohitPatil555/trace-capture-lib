# SPDX-License-Identifier: MIT | Author: Rohit Patil
#!/usr/bin/env bash

set -e

EXEC_DIR=$PWD
COPY_ARTIFACTS=false
EXAMPLE_BUILD=false
IS_STM32_BUILD=false
IS_STM32_RUN=false
REPO_PATH=$PWD
BUILD_DIR=${REPO_PATH}/build

function library_ut_test()
{
    # clean build directory
    rm -rf ${BUILD_DIR}
    mkdir ${BUILD_DIR}
    cd ${BUILD_DIR}

    # config space
    cmake ${REPO_PATH} -DCMAKE_BUILD_TYPE=Debug

    # build cmake
    cmake --build . -- -j$(nproc)

    # build docs
    cmake --build . --target docs

    # Run test
    cd tests/
    ctest --output-on-failure
    cd -

    if [ "$COPY_ARTIFACTS" = "true" ]; then
        sudo cp -r /tmp/build/docs_output  /workspaces/artifacts/
        echo "Copied Artifacts Done"
    fi

    cd $EXEC_DIR
}

function example_build()
{
    SAMPLE_X86=example/sample_x86
    rm -rf ${BUILD_DIR}/${SAMPLE_X86}
    mkdir -p ${BUILD_DIR}/${SAMPLE_X86}
    cd ${BUILD_DIR}/${SAMPLE_X86}

    # config space
    cmake ${REPO_PATH}/${SAMPLE_X86}

    # build cmake
    cmake --build . -- -j$(nproc)

    # execute example
    ./example

    # create trace analysis directory
    mkdir -p traces
    cp generated/metadata traces/
    cp stream.bin traces/

    # Analyse traces
    cd ${REPO_PATH}
    python3 ${REPO_PATH}/${SAMPLE_X86}/post_analysis/post_analysis.py --input_file ${BUILD_DIR}/${SAMPLE_X86}/traces/

    cd $EXEC_DIR
}

function stm32_build()
{
    BARE_METAL=example/stm32
    rm -rf ${BUILD_DIR}/${BARE_METAL}
    mkdir -p ${BUILD_DIR}/${BARE_METAL}
    cd ${BUILD_DIR}/${BARE_METAL}

    # config space
    cmake ${REPO_PATH}/${BARE_METAL} -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake

    # build cmake
    cmake --build . -- -j$(nproc)

    cd $EXEC_DIR
}

# Process named arguments
while [[ "$#" -gt 0 ]]; do
    case "$1" in
        --build-dir)
            BUILD_DIR="$2"
            shift 2 # Move past the --build-dir and its value
            ;;
        --example)
            EXAMPLE_BUILD="true"
            shift 1 # Move past --example
            ;;
        --stm32-build)
            IS_STM32_BUILD=true
            shift 1
            ;;
        --artifacts)
            COPY_ARTIFACTS="true"
            REPO_PATH=${PROJECT_PATH}
            shift 1 # Move past --artifacts
            ;;
        -h|--help)
            echo "Usage: $0 [--build-dir <path>] [--example] [--artifacts]"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

if [ "$EXAMPLE_BUILD" = "true" ]; then
    example_build
elif [ "$IS_STM32_BUILD" = "true" ]; then
    stm32_build
else
    library_ut_test
fi

echo "done"
