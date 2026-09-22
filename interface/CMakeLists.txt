# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/includes)

file(GLOB_RECURSE ZBOOK_INTERFACE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
target_sources(app PRIVATE ${ZBOOK_INTERFACE_SOURCES})

# Custom pinctrl state identifiers for shared, multi-function header IOs
# (see interface/snippets/zbook-sdk/). Per Zephyr's pinctrl docs, a custom
# state's PINCTRL_STATE_<NAME> id must be in scope for *every* translation
# unit that defines or looks up that state -- including the Zephyr driver's
# own .c file -- so it has to be a global compile definition, not a header
# zbook_pwm.c alone includes. One id per custom state name; the next one
# added here should use PINCTRL_STATE_PRIV_START + 1, and so on.
zephyr_compile_definitions(PINCTRL_STATE_ZBOOK_IO=PINCTRL_STATE_PRIV_START)
