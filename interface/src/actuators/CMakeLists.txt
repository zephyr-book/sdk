# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE - 2026

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ZBOOK_INTERFACE_ACTUATORS_LED_RGB zbook_led_rgb.c)
