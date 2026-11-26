#pragma once

#include "GAArtifactStore.h"
#include "GATaskScheduler.h"

// Each scheduler uses the artifact store owned by its workflow session.
class GAWorkflowSession
{
public:
    GAWorkflowSession();
    ~GAWorkflowSession();

    GAWorkflowSession(const GAWorkflowSession&) = delete;
    GAWorkflowSession& operator=(const GAWorkflowSession&) = delete;

    [[nodiscard]] GAArtifactStore& artifacts() noexcept;
    [[nodiscard]] const GAArtifactStore& artifacts() const noexcept;
    [[nodiscard]] IGATaskScheduler& scheduler() noexcept;

private:
    GAArtifactStore artifacts_;
    GAInlineTaskScheduler scheduler_;
};
