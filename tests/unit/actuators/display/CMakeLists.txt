# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE 2026

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(zbook_display_test)


set(ZBOOK_INTERFACE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../interface)

get_property(zephyr_libs GLOBAL PROPERTY ZEPHYR_LIBS)

foreach(lib ${zephyr_libs})
    get_property(lib_dir TARGET ${lib} PROPERTY SOURCE_DIR)
    if (lib_dir STREQUAL ZEPHYR_OLED_DISPLAY_MODULE_DIR)
        set_property(TARGET ${lib} PROPERTY SOURCES ${ZEPHYR_BASE}/misc/empty_file.c)
    endif()
endforeach()

option(ZBOOK_TEST_STATUS_BAR "Build the display assets and the status bar" ON)

target_sources(app PRIVATE
    src/main.c
    src/mock_display.c
    ${ZBOOK_INTERFACE_DIR}/src/actuators/zbook_display.c
)

if (ZBOOK_TEST_STATUS_BAR)
    include(${ZBOOK_INTERFACE_DIR}/assets/assets.cmake)
    zbook_generate_assets(ZBOOK_ASSETS_SRC ZBOOK_ASSETS_INC)
    target_include_directories(app PRIVATE ${ZBOOK_ASSETS_INC})
    target_sources(app PRIVATE
        ${ZBOOK_INTERFACE_DIR}/src/actuators/zbook_display_assets.c
        ${ZBOOK_ASSETS_SRC}
    )
endif()

target_include_directories(app PRIVATE ${ZEPHYR_OLED_DISPLAY_MODULE_DIR}/includes)

target_compile_definitions(app PRIVATE
    CONFIG_ZBOOK_INTERFACE_DISPLAY_DCDC=1
    CONFIG_ZBOOK_INTERFACE_DISPLAY_INIT_CONTRAST=77
    CONFIG_ZBOOK_INTERFACE_ACTUATORS_DISPLAY_ASSETS=$<BOOL:${ZBOOK_TEST_STATUS_BAR}>
    CONFIG_ZBOOK_INTERFACE_ACTUATOR_DISPLAY_STATUS_BAR=$<BOOL:${ZBOOK_TEST_STATUS_BAR}>
)
