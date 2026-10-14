// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT
#pragma once
#include "AeostaraCore/AtomicPersistenceContext.h"
#include <memory>
namespace Aeostara::BaseBindings {
// Local, direct-child path scope. Atomic publication uses checked staging and
// same-directory rename; it does not provide cross-process compare-and-swap or
// a physical power-loss guarantee. Configuration repair uses its held lease.
class WindowsAtomicFileSystem final : public IFileSystem {
public:
    explicit WindowsAtomicFileSystem(Path existingDirectory,
        std::shared_ptr<const AtomicPersistenceContext> persistence = {});
    ~WindowsAtomicFileSystem();
    WindowsAtomicFileSystem(const WindowsAtomicFileSystem&) = delete;
    WindowsAtomicFileSystem& operator=(const WindowsAtomicFileSystem&) = delete;
    OperationAttempt<ByteSequence> read(const Path&) const override;
    OperationAttempt<FileOperationReceipt> writeAtomically(const Path&, const ByteSequence&) override;
    OperationAttempt<FileOperationReceipt> appendAtomically(const Path&, const ByteSequence&) override;
    OperationAttempt<FileOperationReceipt> copyAtomically(const Path&, const Path&) override;
    OperationAttempt<bool> exists(const Path&) const override;
    OperationAttempt<std::string> computeSHA256(const Path&) const override;
    std::vector<BaseContracts::DiagnosticChain> diagnosticChains() const;
private:
    struct State;
    std::unique_ptr<State> state_;
    OperationAttempt<FileOperationReceipt> publish(AtomicOperation, const Path&, const Path&, const ByteSequence&);
};
} // namespace Aeostara::BaseBindings
