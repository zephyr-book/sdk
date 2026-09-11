# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE

cmake_minimum_required(VERSION 3.20.0)

if(NOT DEFINED BOARD)
    set(BOARD zbook@p2/rp2350b/m33)
endif()

set(EXTRA_DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/interface/app.overlay)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(zbook_interface)

target_sources(app PRIVATE main.c)

# interface/ is auto-discovered as a Zephyr module via zephyr/module.yml --
# no add_subdirectory/glob needed here. Language-binding modules (sdk-lua,
# sdk-micropython, ...) attach the same way once added to west.yml.