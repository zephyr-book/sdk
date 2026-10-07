# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE - 2026

zephyr_library()

if(CONFIG_ZBOOK_INTERFACE_IO)
    zephyr_library_sources_ifdef(CONFIG_ZBOOK_INTERFACE_IO_BTN zbook_btn.c)
endif()