#include "GAWorkflowSession.h"

GAWorkflowSession::GAWorkflowSession()
    : scheduler_(artifacts_)
{
}

GAWorkflowSession::~GAWorkflowSession() = default;

GAArtifactStore& GAWorkflowSession::artifacts() noexcept
{
    return artifacts_;
}

const GAArtifactStore& GAWorkflowSession::artifacts() const noexcept
{
    return artifacts_;
}

IGATaskScheduler& GAWorkflowSession::scheduler() noexcept
{
    return scheduler_;
}
