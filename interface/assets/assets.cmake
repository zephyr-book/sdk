# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE 2026

set(ZBOOK_ASSETS_DIR ${CMAKE_CURRENT_LIST_DIR})

function(zbook_generate_assets src_var inc_var)
    set(out_dir ${CMAKE_BINARY_DIR}/zbook_assets)
    file(GLOB bitmaps CONFIGURE_DEPENDS ${ZBOOK_ASSETS_DIR}/bitmaps/*.txt)

    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} ${ZBOOK_ASSETS_DIR}/gen_assets.py --out-dir ${out_dir}
        RESULT_VARIABLE ret
    )
    if(NOT ret EQUAL 0)
        message(FATAL_ERROR "gen_assets.py failed, see the error above")
    endif()

    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        ${ZBOOK_ASSETS_DIR}/gen_assets.py ${bitmaps})

    set(${src_var} ${out_dir}/assets.c PARENT_SCOPE)
    set(${inc_var} ${out_dir}/include PARENT_SCOPE)
endfunction()
