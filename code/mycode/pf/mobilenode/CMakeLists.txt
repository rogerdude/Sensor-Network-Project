# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
set(mcuboot_key_file ${CMAKE_CURRENT_SOURCE_DIR}/mykey.pem)
set(mcuboot_CONFIG_BOOT_SIGNATURE_KEY_FILE "\${mcuboot_key_file}\"")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pf)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../mylib)

target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../mylib/thingy52_gas_colour.c
)
