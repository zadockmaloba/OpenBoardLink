# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

project(blinky)

target_sources(app PRIVATE src/main.c src/drivers/uart.c)
