// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "AeostaraCore/SdkVersion.h"
#if defined(AEOSTARA_UNSUPPORTED_CONFIGURATION)
#error Aeostara supports only Debug and Release configurations
#endif

#if !defined(_WIN32) || !defined(_MSC_VER)
#error Aeostara SDK requires Windows and the qualified MSVC compiler.
#endif
#if !defined(_M_X64) || defined(_M_ARM64) || defined(_M_ARM64EC)
#error Aeostara SDK supports Windows x64 only.
#endif
#if _MSC_FULL_VER != 194435223
#error Aeostara SDK requires qualified MSVC 19.44.35223 (v143).
#endif
#if !defined(_MSVC_LANG) || _MSVC_LANG < 202002L
#error Aeostara SDK requires C++20 or later language mode.
#endif
#if !defined(_DLL)
#error Aeostara SDK requires the dynamic CRT: /MD Release or /MDd Debug.
#endif
#if !defined(_CPPUNWIND) || !defined(_CPPRTTI)
#error Aeostara SDK requires exception unwinding (/EHsc) and RTTI (/GR).
#endif

#include <vector>

#ifdef _DEBUG
#if _ITERATOR_DEBUG_LEVEL != 2
#error Aeostara Debug SDK requires iterator debug level 2.
#endif
#if defined(AEOSTARA_EXPECT_DEBUG) && AEOSTARA_EXPECT_DEBUG != 1
#error Aeostara Debug client selected a Release SDK binary.
#endif
#pragma detect_mismatch("Aeostara.Configuration", "Debug")
#pragma detect_mismatch("Aeostara.CRT", "MDd")
#else
#if _ITERATOR_DEBUG_LEVEL != 0
#error Aeostara Release SDK requires iterator debug level 0.
#endif
#if defined(AEOSTARA_EXPECT_DEBUG) && AEOSTARA_EXPECT_DEBUG != 0
#error Aeostara Release client selected a Debug SDK binary.
#endif
#pragma detect_mismatch("Aeostara.Configuration", "Release")
#pragma detect_mismatch("Aeostara.CRT", "MD")
#endif
#pragma detect_mismatch("Aeostara.Compiler", "194435223")
#pragma detect_mismatch("Aeostara.SDK", AEOSTARA_SDK_VERSION)
