# Aeostara CLI — Executable
# Copyright (c) 2026 James Daley, Raven Forge Software.
# SPDX-License-Identifier: MIT

add_executable(aeostara
    Main.cpp
    CommandParser.cpp
    CLIRunner.cpp
)

target_include_directories(aeostara PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(aeostara PRIVATE AeostaraCore)
