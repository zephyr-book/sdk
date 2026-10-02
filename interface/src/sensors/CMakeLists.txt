# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE - 2026

zephyr_library()

if(CONFIG_ZBOOK_INTERFACE_SENSORS)
    zephyr_library_sources_ifdef(CONFIG_ZBOOK_INTERFACE_LDR zbook_ldr.c)
endif()