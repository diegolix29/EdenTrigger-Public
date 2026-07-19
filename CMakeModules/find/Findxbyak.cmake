# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

include(FindPackageHandleStandardArgs)

# If xbyak_INCLUDE_DIR is already set (by parent project), use it
if (NOT xbyak_INCLUDE_DIR)
    find_path(xbyak_INCLUDE_DIR xbyak.h
        PATH_SUFFIXES xbyak
        PATHS
            ${CMAKE_SOURCE_DIR}/.cache/cpm/xbyak/7.35.2/xbyak
            ${CMAKE_SOURCE_DIR}/.cache/cpm/xbyak/v7.35.2/xbyak
            /usr/include
            /usr/local/include
    )
endif()

find_package_handle_standard_args(xbyak
    REQUIRED_VARS xbyak_INCLUDE_DIR
)

if (xbyak_FOUND AND NOT TARGET xbyak::xbyak)
    add_library(xbyak::xbyak INTERFACE IMPORTED)
    set_property(TARGET xbyak::xbyak PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES "${xbyak_INCLUDE_DIR}"
    )
endif()
