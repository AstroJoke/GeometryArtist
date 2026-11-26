#include "GATaskScheduler.h"

#include "IGAArtifactRepository.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

class GAInlineTaskScheduler::Impl
{
public:
    struct TaskRecord
    {
        GATaskStatus status;
        IGATaskNode* node;
        bool cancelRequested = false;
        bool callbackRunning = false;
    };

    explicit Impl(IGAArtifactRepository& artifactRepository)
        : artifacts(artifactRepository)
    {
    }

    IGAArtifactRepository& artifacts;
    mutable std::shared_mutex mutex;
    std::unordered_map<GATaskId, TaskRecord> tasks;
    GATaskId nextId = 1;
    std::size_t running = 0;
    std::condition_variable_any finished;

    template <typename Predicate>
    bool waitFor(std::unique_lock<std::shared_mutex>& lock, std::uint64_t milliseconds, Predicate done)
    {
        if (milliseconds == 0)
        {
            return done();
        }
        // Bound each relative duration so very large timeouts fit the platform clock.
        constexpr auto day = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::hours(24));
        while (milliseconds > static_cast<std::uint64_t>(day.count()))
        {
            if (finished.wait_for(lock, day, done))
            {
                return true;
            }
            milliseconds -= day.count();
        }
        return finished.wait_for(lock, std::chrono::milliseconds(milliseconds), done);
    }
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
        impl_->tasks.emplace(id, Impl::TaskRecord{{id, GATaskState::Running, 0.0, "Task is running"}, &node});
        ++impl_->running;
    }

    bool succeeded = false;
    bool threw = false;
    try
    {
        succeeded = node.execute(impl_->artifacts);
    }
    catch (...)
    {
        // A failed node is reported through its task status.
        threw = true;
    }

    {
        std::unique_lock lock(impl_->mutex);
        auto& record = impl_->tasks.at(id);
        record.node = nullptr;
        // The caller can destroy the borrowed node as soon as submit returns.
        impl_->finished.wait(lock, [&record] { return !record.callbackRunning; });
        --impl_->running;
        impl_->finished.notify_all();
        auto& status = record.status;
        status.state = succeeded ? GATaskState::Completed : GATaskState::Failed;
        status.progress = succeeded ? 1.0 : 0.0;
        status.message = succeeded ? "Task completed" : "Task failed";
        if (record.cancelRequested && !succeeded && !threw)
        {
            status.state = GATaskState::Cancelled;
            status.message = "Task cancelled";
        }
    }
    return id;
}

void GAInlineTaskScheduler::cancel(GATaskId task) noexcept
{
    std::unique_lock lock(impl_->mutex);
    const auto found = impl_->tasks.find(task);
    if (found == impl_->tasks.end())
    {
        return;
    }
    auto& record = found->second;
    if (!record.node || record.cancelRequested)
    {
        return;
    }
    auto& node = *record.node;
    record.cancelRequested = true;
    record.callbackRunning = true;
    lock.unlock();
    node.cancel();
    lock.lock();
    record.callbackRunning = false;
    impl_->finished.notify_all();
}

void GAInlineTaskScheduler::cancelAll()
{
    std::vector<GATaskId> active;
    {
        std::shared_lock lock(impl_->mutex);
        active.reserve(impl_->running);
        for (const auto& [id, record] : impl_->tasks)
        {
            if (record.node && !record.cancelRequested)
            {
                active.push_back(id);
            }
        }
    }
    for (const auto id : active)
    {
        cancel(id);
    }
}

GATaskStatus GAInlineTaskScheduler::status(GATaskId task) const
{
    std::shared_lock lock(impl_->mutex);
    const auto found = impl_->tasks.find(task);
    return found == impl_->tasks.end() ? GATaskStatus{GAInvalidTaskId, GATaskState::Failed, 0.0, "Task not found"}
                                       : found->second.status;
}

bool GAInlineTaskScheduler::wait(GATaskId task, std::uint64_t timeoutMilliseconds)
{
    std::unique_lock lock(impl_->mutex);
    const auto found = impl_->tasks.find(task);
    if (found == impl_->tasks.end())
    {
        return false;
    }
    // Records live for the session. References survive insertions during a wait.
    const auto& record = found->second;
    return impl_->waitFor(lock, timeoutMilliseconds, [&record] { return record.status.state != GATaskState::Running; });
}

bool GAInlineTaskScheduler::waitUntilIdle(std::uint64_t timeoutMilliseconds)
{
    std::unique_lock lock(impl_->mutex);
    return impl_->waitFor(lock, timeoutMilliseconds, [this] { return impl_->running == 0; });
}
