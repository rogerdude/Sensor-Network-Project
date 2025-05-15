cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(prac2)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../mylib)

target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../mylib/rtc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../mylib/sltb_sensors.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../mylib/sample.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../mylib/thunderboard_button.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/board_init.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
)

