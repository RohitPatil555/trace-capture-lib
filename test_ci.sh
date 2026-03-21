# SPDX-License-Identifier: MIT | Author: Rohit Patil
#!/usr/bin/env bash

set -e

EXEC_DIR=$PWD
BUILD_DIR=build
COPY_ARTIFACTS=false
EXAMPLE_BUILD=false
REPO_PATH=$PWD

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
    rm -rf ${BUILD_DIR}/example
    mkdir -p ${BUILD_DIR}/example
    cd ${BUILD_DIR}/example

    # config space
    cmake ${REPO_PATH}/example

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
    python3 ${REPO_PATH}/example/post_analysis/post_analysis.py --input_file ${BUILD_DIR}/example/traces/

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
else
    library_ut_test
fi

echo "done"
