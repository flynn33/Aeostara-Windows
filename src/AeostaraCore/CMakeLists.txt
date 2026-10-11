# Aeostara
# Copyright (c) 2026 James Daley. All Rights Reserved.
add_library(AeostaraCore STATIC
    Version.cpp Contracts.cpp Invariant.cpp InvariantParser.cpp JsonPath.cpp DriftAnalyzer.cpp
    RepairPlanner.cpp PolicyEvaluator.cpp BackupManager.cpp Verification.cpp RollbackManager.cpp
    AuditTrail.cpp HealingEngine.cpp JsonConfigAdapter.cpp FileSystem.cpp Json.cpp BaseContracts.cpp
    SemanticMapping.cpp AshBinding.cpp BasePolicyGate.cpp BaseDocumentBindings.cpp BaseAuditBindings.cpp NativeRequestBinding.cpp WindowsAtomicFileSystem.cpp NativeExecutionBindings.cpp
    ../AeostaraDiagnostics/Diagnostics.cpp)
if(AEOSTARA_DIAGNOSTIC_PROFILE STREQUAL "LEAN")
    target_sources(AeostaraCore PRIVATE ../AeostaraDiagnostics/DiagnosticsLean.cpp)
else()
    target_sources(AeostaraCore PRIVATE ../AeostaraDiagnostics/DiagnosticsDevelopment.cpp)
endif()
set_target_properties(AeostaraCore PROPERTIES EXPORT_NAME Core DEBUG_POSTFIX ""
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    INTERPROCEDURAL_OPTIMIZATION OFF)
add_library(Aeostara::Core ALIAS AeostaraCore)
target_compile_features(AeostaraCore PUBLIC cxx_std_20)
execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE AEOSTARA_BUILD_SOURCE_SHA OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE AEOSTARA_BUILD_SOURCE_RESULT)
execute_process(COMMAND git status --porcelain WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE AEOSTARA_BUILD_DIRTY OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE AEOSTARA_BUILD_DIRTY_RESULT)
if(AEOSTARA_BUILD_SOURCE_RESULT EQUAL 0 AND AEOSTARA_BUILD_SOURCE_SHA MATCHES "^[0-9a-f]+$")
    target_compile_definitions(AeostaraCore PRIVATE AEOSTARA_SOURCE_COMMIT="${AEOSTARA_BUILD_SOURCE_SHA}")
endif()
if(AEOSTARA_BUILD_DIRTY_RESULT EQUAL 0 AND AEOSTARA_BUILD_DIRTY STREQUAL "")
    target_compile_definitions(AeostaraCore PRIVATE AEOSTARA_SOURCE_DIRTY=0)
elseif(AEOSTARA_BUILD_DIRTY_RESULT EQUAL 0)
    target_compile_definitions(AeostaraCore PRIVATE AEOSTARA_SOURCE_DIRTY=1)
endif()
target_compile_definitions(AeostaraCore PUBLIC
    "AEOSTARA_EXPECT_DEBUG=$<IF:$<STREQUAL:$<CONFIG>,Debug>,1,0>"
    "$<$<NOT:$<OR:$<STREQUAL:$<CONFIG>,Debug>,$<STREQUAL:$<CONFIG>,Release>>>:AEOSTARA_UNSUPPORTED_CONFIGURATION=1>")
target_compile_options(AeostaraCore PUBLIC /FIAeostaraCore/SdkCompatibility.h)
target_include_directories(AeostaraCore PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>" "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>" "$<INSTALL_INTERFACE:include>")
target_link_libraries(AeostaraCore PUBLIC bcrypt ole32)
if(AEOSTARA_INSTALL_SDK)
    include(CMakePackageConfigHelpers)
    install(TARGETS AeostaraCore EXPORT AeostaraTargets ARCHIVE DESTINATION "lib/x64/$<CONFIG>")
    set(AEOSTARA_PUBLIC_HEADERS
        HealingEngine.h IHealingEngine.h Contracts.h Invariant.h IConfigAdapter.h IBackupProvider.h
        IAuditSink.h IFileSystem.h JsonConfigAdapter.h BackupManager.h AuditTrail.h Version.h
        Json.h BaseContracts.h BaseAdapterBindings.h BaseDocumentBindings.h BaseAuditBindings.h NativeRequestBinding.h NativeExecutionBindings.h AtomicPersistenceContext.h WindowsAtomicFileSystem.h SemanticMapping.h IAshBindingProvider.h BasePolicyGate.h
        Diagnostics.h SdkCompatibility.h)
    foreach(header IN LISTS AEOSTARA_PUBLIC_HEADERS)
        install(FILES "${PROJECT_SOURCE_DIR}/include/AeostaraCore/${header}" DESTINATION include/AeostaraCore)
    endforeach()
    install(FILES "${PROJECT_BINARY_DIR}/include/AeostaraCore/SdkVersion.h" DESTINATION include/AeostaraCore)
    configure_package_config_file("${PROJECT_SOURCE_DIR}/cmake/AeostaraConfig.cmake.in"
        "${PROJECT_BINARY_DIR}/AeostaraConfig.cmake" INSTALL_DESTINATION lib/cmake/Aeostara)
    write_basic_package_version_file("${PROJECT_BINARY_DIR}/AeostaraConfigVersion.cmake"
        VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMajorVersion)
    install(FILES "${PROJECT_BINARY_DIR}/AeostaraConfig.cmake" "${PROJECT_BINARY_DIR}/AeostaraConfigVersion.cmake"
        DESTINATION lib/cmake/Aeostara)
    install(EXPORT AeostaraTargets NAMESPACE Aeostara:: DESTINATION lib/cmake/Aeostara)
    install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" "${PROJECT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md" DESTINATION share/aeostara)
endif()
