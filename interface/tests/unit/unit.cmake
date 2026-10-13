# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE - 2026
#
# Shared boilerplate for interface unit tests (tests/unit/<category>/<peripheral>/).
# Include from a test's CMakeLists.txt and call zbook_unit_test(<name>) once:
#
#   cmake_minimum_required(VERSION 3.20.0)
#   include(${CMAKE_CURRENT_SOURCE_DIR}/../../unit.cmake)
#   zbook_unit_test(i2c)
macro(zbook_unit_test name)
	find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
	project(test_zbook_${name})

	target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c)
endmacro()
