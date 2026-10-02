# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_zbook_potentiometer)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../unit.cmake)
zbook_unit_test(potentiometer)
