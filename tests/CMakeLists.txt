# Aeostara Tests — Microsoft CppUnitTest
# Copyright (c) 2026 James Daley. All Rights Reserved.
#
# Tests use Microsoft CppUnitTest (native Visual Studio framework).
# Tests are built as DLLs and discovered by Visual Studio Test Explorer and vstest.console.exe.

if(MSVC)
    set(VSTEST_CONSOLE_EXE "")

    if(DEFINED CMAKE_GENERATOR_INSTANCE AND
       EXISTS "${CMAKE_GENERATOR_INSTANCE}/Common7/IDE/Extensions/TestPlatform/vstest.console.exe")
        set(VSTEST_CONSOLE_EXE
            "${CMAKE_GENERATOR_INSTANCE}/Common7/IDE/Extensions/TestPlatform/vstest.console.exe")
    elseif(DEFINED ENV{VSINSTALLDIR} AND NOT "$ENV{VSINSTALLDIR}" STREQUAL "" AND
           EXISTS "$ENV{VSINSTALLDIR}Common7/IDE/Extensions/TestPlatform/vstest.console.exe")
        set(VSTEST_CONSOLE_EXE
            "$ENV{VSINSTALLDIR}Common7/IDE/Extensions/TestPlatform/vstest.console.exe")
    endif()

    if(VSTEST_CONSOLE_EXE STREQUAL "")
        find_program(
            VSTEST_CONSOLE_EXE
            NAMES vstest.console.exe
            HINTS
                "${CMAKE_GENERATOR_INSTANCE}/Common7/IDE/Extensions/TestPlatform"
                "$ENV{VSINSTALLDIR}Common7/IDE/Extensions/TestPlatform"
        )
    endif()

    if(NOT VSTEST_CONSOLE_EXE)
        message(FATAL_ERROR "Unable to locate vstest.console.exe for CppUnitTest registration.")
    endif()

    # Visual Studio CppUnitTest framework paths
    if(DEFINED ENV{VCInstallDir} AND NOT "$ENV{VCInstallDir}" STREQUAL "")
        set(VC_INSTALL_DIR "$ENV{VCInstallDir}")
    else()
        get_filename_component(_CL_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        get_filename_component(_CL_DIR "${_CL_DIR}" DIRECTORY)
        get_filename_component(_CL_DIR "${_CL_DIR}" DIRECTORY)
        get_filename_component(_CL_DIR "${_CL_DIR}" DIRECTORY)
        get_filename_component(_CL_DIR "${_CL_DIR}" DIRECTORY)
        get_filename_component(_CL_DIR "${_CL_DIR}" DIRECTORY)
        get_filename_component(VC_INSTALL_DIR "${_CL_DIR}" DIRECTORY)
        set(VC_INSTALL_DIR "${VC_INSTALL_DIR}/")
    endif()
    set(VS_UNITTEST_INCLUDE "${VC_INSTALL_DIR}Auxiliary/VS/UnitTest/include")
    set(VS_UNITTEST_LIB "${VC_INSTALL_DIR}Auxiliary/VS/UnitTest/lib/x64")

    # --- AeostaraCoreTests ---
    add_library(AeostaraCoreTests SHARED
        AeostaraCoreTests/ContractsTests.cpp
        AeostaraCoreTests/JsonPathTests.cpp
        AeostaraCoreTests/InvariantParserTests.cpp
        AeostaraCoreTests/DriftAnalyzerTests.cpp
        AeostaraCoreTests/RepairPlannerTests.cpp
        AeostaraCoreTests/PolicyEvaluatorTests.cpp
        AeostaraCoreTests/BackupManagerTests.cpp
        AeostaraCoreTests/VerificationTests.cpp
        AeostaraCoreTests/RollbackManagerTests.cpp
        AeostaraCoreTests/AuditTrailTests.cpp
        AeostaraCoreTests/HealingEngineTests.cpp
        AeostaraCoreTests/JsonConfigAdapterTests.cpp
        AeostaraCoreTests/JsonValueTests.cpp
        AeostaraCoreTests/BaseContractsTests.cpp
        AeostaraCoreTests/AuditSafetyTests.cpp
        AeostaraCoreTests/BaseDocumentBindingsTests.cpp
        AeostaraCoreTests/BaseAuditBindingsTests.cpp
        AeostaraCoreTests/SemanticMappingTests.cpp
        AeostaraCoreTests/JsonAggregateTests.cpp
        AeostaraCoreTests/WindowsAtomicFileSystemTests.cpp
        NativeExecutionBindingsTests.cpp
    )
    target_include_directories(AeostaraCoreTests PRIVATE
        ${VS_UNITTEST_INCLUDE}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/AeostaraCoreTests
    )
    target_link_directories(AeostaraCoreTests PRIVATE ${VS_UNITTEST_LIB})
    target_compile_definitions(AeostaraCoreTests PRIVATE
        AEOSTARA_EXECUTION_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/adopted-semantic/proposed_deterministic_fixtures.json"
        AEOSTARA_EXECUTION_AUTHORITY="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/adopted-semantic/proposed_source_authority.json")
    target_link_libraries(AeostaraCoreTests PRIVATE
        Microsoft.VisualStudio.TestTools.CppUnitTestFramework
        AeostaraCore
    )

    add_test(
        NAME AeostaraCoreTests
        COMMAND "${VSTEST_CONSOLE_EXE}" "$<TARGET_FILE:AeostaraCoreTests>" /Platform:x64
            "/ResultsDirectory:${CMAKE_BINARY_DIR}/TestResults/$<CONFIG>" "/Logger:trx;LogFileName=AeostaraCoreTests.trx"
    )

    # --- AeostaraArchitectureTests ---
    add_library(AeostaraArchitectureTests SHARED
        AeostaraArchitectureTests/ArchitectureEnforcementTests.cpp
    )
    target_include_directories(AeostaraArchitectureTests PRIVATE
        ${VS_UNITTEST_INCLUDE}
        ${CMAKE_SOURCE_DIR}/include
    )
    target_compile_definitions(AeostaraArchitectureTests PRIVATE
        AEOSTARA_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    )
    target_link_directories(AeostaraArchitectureTests PRIVATE ${VS_UNITTEST_LIB})
    target_link_libraries(AeostaraArchitectureTests PRIVATE
        Microsoft.VisualStudio.TestTools.CppUnitTestFramework
    )

    add_test(
        NAME AeostaraArchitectureTests
        COMMAND "${VSTEST_CONSOLE_EXE}" "$<TARGET_FILE:AeostaraArchitectureTests>" /Platform:x64
            "/ResultsDirectory:${CMAKE_BINARY_DIR}/TestResults/$<CONFIG>" "/Logger:trx;LogFileName=AeostaraArchitectureTests.trx"
    )

    # --- AeostaraCLITests ---
    add_library(AeostaraCLITests SHARED
        AeostaraCLITests/CommandParserTests.cpp
        AeostaraCLITests/CLIRunnerTests.cpp
        ${CMAKE_SOURCE_DIR}/src/AeostaraCLI/CommandParser.cpp
        ${CMAKE_SOURCE_DIR}/src/AeostaraCLI/CLIRunner.cpp
    )
    target_include_directories(AeostaraCLITests PRIVATE
        ${VS_UNITTEST_INCLUDE}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/AeostaraCLI
    )
    target_link_directories(AeostaraCLITests PRIVATE ${VS_UNITTEST_LIB})
    target_link_libraries(AeostaraCLITests PRIVATE
        Microsoft.VisualStudio.TestTools.CppUnitTestFramework
        AeostaraCore
    )

    add_test(
        NAME AeostaraCLITests
        COMMAND "${VSTEST_CONSOLE_EXE}" "$<TARGET_FILE:AeostaraCLITests>" /Platform:x64
            "/ResultsDirectory:${CMAKE_BINARY_DIR}/TestResults/$<CONFIG>" "/Logger:trx;LogFileName=AeostaraCLITests.trx"
    )
endif()

foreach(suite SemanticConformanceTests LifecycleConformanceTests)
    add_executable(${suite} conformance/${suite}.cpp)
    target_link_libraries(${suite} PRIVATE AeostaraCore)
    add_test(NAME ${suite} COMMAND ${suite}
        "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/adopted-semantic/proposed_deterministic_fixtures.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/adopted-semantic/proposed_source_authority.json")
endforeach()

add_executable(DiagnosticFoundationTests diagnostics/DiagnosticFoundationTests.cpp)
target_link_libraries(DiagnosticFoundationTests PRIVATE AeostaraCore)
if(AEOSTARA_DIAGNOSTIC_PROFILE STREQUAL "DEVELOPMENT")
    add_test(NAME DiagnosticFoundationTests COMMAND DiagnosticFoundationTests development)
else()
    add_test(NAME DiagnosticFoundationTests COMMAND DiagnosticFoundationTests lean)
endif()
