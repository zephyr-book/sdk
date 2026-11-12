# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE - 2026

zephyr_include_directories(includes)

# TODO @JoaoMatheusND: remove once oled-sh1106-zephyr (deps/display) guards its sources with
# CONFIG_OLED_DISPLAY. It compiles display.c unconditionally and display.h
# includes fonts.h, but oled-mono-fonts only exports its include dir when
# CONFIG_OLED_FONTS=y, so non-display apps fail with "fonts.h: No such file
# or directory". Exporting the fonts headers globally works around it. 

if(DEFINED ZEPHYR_OLED_FONTS_MODULE_DIR)
    zephyr_include_directories(${ZEPHYR_OLED_FONTS_MODULE_DIR}/includes)
endif()

add_subdirectory_ifdef(CONFIG_ZBOOK_INTERFACE_ACTUATORS src/actuators)
add_subdirectory_ifdef(CONFIG_ZBOOK_INTERFACE_IO src/io)
add_subdirectory_ifdef(CONFIG_ZBOOK_INTERFACE_PROTOCOLS src/protocols)
add_subdirectory_ifdef(CONFIG_ZBOOK_INTERFACE_SENSORS src/sensors)
add_subdirectory_ifdef(CONFIG_ZBOOK_INTERFACE_STORAGE src/storage)
