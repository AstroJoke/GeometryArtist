#pragma once

#include "GAWorkbenchTypes.h"

#include <memory>

class IGAArtifactRepository;

class IGATaskNode
{
public:
    virtual ~IGATaskNode() = default;

    virtual bool execute(IGAArtifactRepository& artifacts) = 0;
};

class IGATaskScheduler
{
public:
    virtual ~IGATaskScheduler() = default;

    [[nodiscard]] virtual GATaskId submit(IGATaskNode& node) = 0;
    [[nodiscard]] virtual GATaskStatus status(GATaskId task) const = 0;
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
    [[nodiscard]] GATaskStatus status(GATaskId task) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
