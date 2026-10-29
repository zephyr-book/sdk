# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE 2026

zephyr_library()

if (CONFIG_ZBOOK_INTERFACE_ACTUATORS)
    zephyr_library_sources_ifdef(CONFIG_ZBOOK_INTERFACE_ACTUATORS_DISPLAY zbook_display.c)

    zephyr_library_sources_ifdef(CONFIG_ZBOOK_INTERFACE_ACTUATOR_DISPLAY_STATUS_BAR
        zbook_display_assets.c)

    if (CONFIG_ZBOOK_INTERFACE_ACTUATORS_DISPLAY_ASSETS)
        include(${CMAKE_CURRENT_LIST_DIR}/../../assets/assets.cmake)
        zbook_generate_assets(ZBOOK_ASSETS_SRC ZBOOK_ASSETS_INC)
        zephyr_include_directories(${ZBOOK_ASSETS_INC})
        zephyr_library_sources(${ZBOOK_ASSETS_SRC})
    endif()
endif()