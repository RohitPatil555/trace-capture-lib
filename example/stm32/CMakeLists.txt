# SPDX-License-Identifier: MIT | Author: Rohit Patil
# --------------------------------------------------------------------
# CMakeLists.txt
# --------------------------------------------------------------------
cmake_minimum_required(VERSION 3.25)
project(stm32 LANGUAGES CXX)

set(CMAKE_CXX_FLAGS "-march=armv7-m -fno-exceptions -fno-rtti -Wall -O0 -g")
set(CMAKE_C_FLAGS   "-march=armv7-m -Wall -O0 -g")

# Linker script
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)

add_executable(${PROJECT_NAME}.elf
    src/main.cpp
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    -nostdlib
    -T${LINKER_SCRIPT}
)
