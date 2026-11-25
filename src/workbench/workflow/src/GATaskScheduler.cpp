#include "GATaskScheduler.h"

#include "IGAArtifactRepository.h"

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

class GAInlineTaskScheduler::Impl
{
public:
    explicit Impl(IGAArtifactRepository& artifactRepository)
        : artifacts(artifactRepository)
    {
    }

    IGAArtifactRepository& artifacts;
    mutable std::shared_mutex mutex;
    std::unordered_map<GATaskId, GATaskStatus> tasks;
    GATaskId nextId = 1;
};

GAInlineTaskScheduler::GAInlineTaskScheduler(IGAArtifactRepository& artifacts)
    : impl_(std::make_unique<Impl>(artifacts))
{
}

GAInlineTaskScheduler::~GAInlineTaskScheduler() = default;

GATaskId GAInlineTaskScheduler::submit(IGATaskNode& node)
{
    GATaskId id = GAInvalidTaskId;
    {
        std::unique_lock lock(impl_->mutex);
        id = impl_->nextId++;
        impl_->tasks.emplace(id, GATaskStatus{id, GATaskState::Running, 0.0, "Task is running"});
    }

    bool succeeded = false;
    try
    {
        succeeded = node.execute(impl_->artifacts);
    }
    catch (...)
    {
        // A failed node is reported through its task status.
        succeeded = false;
    }

    {
        std::unique_lock lock(impl_->mutex);
        auto& status = impl_->tasks.at(id);
        status.state = succeeded ? GATaskState::Completed : GATaskState::Failed;
        status.progress = succeeded ? 1.0 : 0.0;
        status.message = succeeded ? "Task completed" : "Task failed";
    }
    return id;
}

GATaskStatus GAInlineTaskScheduler::status(GATaskId task) const
{
    std::shared_lock lock(impl_->mutex);
    const auto found = impl_->tasks.find(task);
    return found == impl_->tasks.end() ? GATaskStatus{GAInvalidTaskId, GATaskState::Failed, 0.0, "Task not found"}
                                       : found->second;
}
