# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE - 2026

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ZBOOK_INTERFACE_SENSORS_MICROPHONE zbook_microphone.c)
