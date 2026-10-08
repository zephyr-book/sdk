# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE

zephyr_library()

if(CONFIG_ZBOOK_INTERFACE_PROTOCOLS)
  zephyr_library_sources_ifdef(CONFIG_ZBOOK_INTERFACE_PROTOCOLS_UART zbook_uart.c)
endif()