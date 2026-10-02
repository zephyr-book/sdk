# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE

cmake_minimum_required(VERSION 3.20.0)

# Test-only binding for the always-fails-init fixture (dts/bindings/serial/
# vnd,zbook-test-fail-uart.yaml) -- must be set before find_package(Zephyr).
list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_zbook_uart)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../unit.cmake)
zbook_unit_test()
