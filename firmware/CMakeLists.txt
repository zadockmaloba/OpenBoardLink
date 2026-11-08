# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

zephyr_linker_sources(SECTIONS at_cmd_iterable.ld)
#zephyr_iterable_section(NAME at_cmd GROUP RODATA_REGION SUBALIGN 4)

list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

project(OpenBoardLink)

target_include_directories(app
PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers
  ${CMAKE_CURRENT_SOURCE_DIR}/src/lib
)

target_sources(app 
PRIVATE 
  src/main.c 
  src/tasks.c
  src/drivers/swd.h
  src/drivers/swd.c
  src/drivers/uart.h
  src/drivers/uart.c
  src/lib/at_commands.h
  src/lib/at_commands.c
)
