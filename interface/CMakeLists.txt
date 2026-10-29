# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE - 2026

zephyr_include_directories(includes)

add_subdirectory_ifdef(CONFIG_ZBOOK_INTERFACE_ACTUATORS src/actuator)
add_subdirectory_ifdef(CONFIG_ZBOOK_INTERFACE_IO src/io)
add_subdirectory_ifdef(CONFIG_ZBOOK_INTERFACE_PROTOCOLS src/protocols)
add_subdirectory_ifdef(CONFIG_ZBOOK_INTERFACE_SENSORS src/sensors)
add_subdirectory_ifdef(CONFIG_ZBOOK_INTERFACE_STORAGE src/storage)
