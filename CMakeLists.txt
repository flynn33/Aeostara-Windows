# Aeostara — Self-Healing Configuration Platform
# Copyright (c) 2026 James Daley. All Rights Reserved.
# Proprietary and Confidential.

cmake_minimum_required(VERSION 3.28)
project(Aeostara
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Aeostara — Deterministic Config Drift Healing CLI"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC warnings as errors
if(MSVC)
    add_compile_options(/W4 /WX)
endif()

# Find nlohmann/json via vcpkg
find_package(nlohmann_json CONFIG REQUIRED)

# Include directories for public headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Library targets
add_subdirectory(src/AeostaraCore)

# CLI executable
add_subdirectory(src/AeostaraCLI)

# Tests
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
