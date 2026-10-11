// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
#pragma once
#include "AeostaraCore/SemanticMapping.h"
#include "AeostaraCore/IFileSystem.h"
#include "AeostaraCore/IConfigAdapter.h"
namespace Aeostara {
// The caller supplies complete adopted semantic authority, an explicitly
// authorized target and selected recovery plan. This read-only binding captures
// that target's actual bytes/identity/version and binds existing plan digests.
// It does not select a recovery target, invent a witness, or authorize a policy.
OperationAttempt<Json> bindCapturedNativeTarget(const Json& authorizedCompleteInput,
    const std::string& authorizedTargetPath, IFileSystem& filesystem, IConfigAdapter& adapter);
} // namespace Aeostara
