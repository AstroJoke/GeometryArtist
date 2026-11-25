#pragma once

#include "GAWorkbenchTypes.h"

#include <memory>

class IGAArtifactRepository;

class IGATaskNode
{
public:
    virtual ~IGATaskNode() = default;

    // May run concurrently with execute(). Return promptly without waiting for completion.
    virtual void cancel() noexcept {}
    // After a cancel request, false reports cancellation, true reports completion, and an exception reports failure.
    virtual bool execute(IGAArtifactRepository& artifacts) = 0;
};

class IGATaskScheduler
{
public:
    virtual ~IGATaskScheduler() = default;

    [[nodiscard]] virtual GATaskId submit(IGATaskNode& node) = 0;
    virtual void cancel(GATaskId task) noexcept = 0;
    virtual void cancelAll() = 0;
    [[nodiscard]] virtual GATaskStatus status(GATaskId task) const = 0;
    // A zero timeout polls. Unknown task IDs return false immediately.
    [[nodiscard]] virtual bool wait(GATaskId task, std::uint64_t timeoutMilliseconds) = 0;
    [[nodiscard]] virtual bool waitUntilIdle(std::uint64_t timeoutMilliseconds) = 0;
};

// submit() executes the borrowed node on the calling thread before returning its ID.
class GAInlineTaskScheduler : public IGATaskScheduler
{
public:
    explicit GAInlineTaskScheduler(IGAArtifactRepository& artifacts);
    ~GAInlineTaskScheduler() override;

    GAInlineTaskScheduler(const GAInlineTaskScheduler&) = delete;
    GAInlineTaskScheduler& operator=(const GAInlineTaskScheduler&) = delete;

    [[nodiscard]] GATaskId submit(IGATaskNode& node) override;
    void cancel(GATaskId task) noexcept override;
    void cancelAll() override;
    [[nodiscard]] GATaskStatus status(GATaskId task) const override;
    [[nodiscard]] bool wait(GATaskId task, std::uint64_t timeoutMilliseconds) override;
    [[nodiscard]] bool waitUntilIdle(std::uint64_t timeoutMilliseconds) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
