# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE

cmake_minimum_required(VERSION 3.20.0)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../unit.cmake)
zbook_unit_test(led_rgb)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/zbook_led_rgb_fake_strip.c)
