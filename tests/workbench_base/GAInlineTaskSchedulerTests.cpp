#include "GAWorkflowSession.h"

#include <atomic>
#include <chrono>
#include <future>
#include <limits>
#include <stdexcept>

namespace
{
class LookupTask : public IGATaskNode
{
public:
    explicit LookupTask(GAArtifactId artifact) : artifact_(artifact) {}

    bool execute(IGAArtifactRepository& artifacts) override
    {
        if (artifact_ == GAInvalidArtifactId)
        {
            throw std::invalid_argument("Missing artifact ID");
        }
        return artifacts.dataHandle(artifact_) != GAInvalidDataHandle;
    }

private:
    GAArtifactId artifact_;
};

class BlockingTask : public IGATaskNode
{
public:
    bool execute(IGAArtifactRepository&) override
    {
        started.set_value();
        release.get_future().wait();
        return true;
    }

    std::promise<void> started;
    std::promise<void> release;
};

class CancellableTask : public IGATaskNode
{
public:
    explicit CancellableTask(bool succeeds) : succeeds_(succeeds) {}

    bool execute(IGAArtifactRepository&) override
    {
        started.set_value();
        cancelEntered.get_future().wait();
        leaving.set_value();
        return succeeds_;
    }

    void cancel() noexcept override
    {
        if (++requests == 1)
        {
            cancelEntered.set_value();
            releaseCallback.get_future().wait();
        }
    }

    std::promise<void> started;
    std::promise<void> leaving;
    std::promise<void> releaseCallback;
    std::atomic<int> requests{0};

private:
    std::promise<void> cancelEntered;
    bool succeeds_;
};

bool checkCancellation(IGATaskScheduler& scheduler, bool succeeds)
{
    using namespace std::chrono_literals;
    CancellableTask task(succeeds);
    auto started = task.started.get_future();
    auto leaving = task.leaving.get_future();
    auto submitted = std::async(std::launch::async, [&] { return scheduler.submit(task); });
    started.wait();
    auto cancelling = std::async(std::launch::async, [&] { scheduler.cancelAll(); });
    leaving.wait();
    scheduler.cancelAll();
    const bool stillBorrowed = submitted.wait_for(10ms) == std::future_status::timeout;
    const bool stillBusy = !scheduler.waitUntilIdle(0);
    task.releaseCallback.set_value();
    cancelling.get();
    const auto id = submitted.get();
    scheduler.cancel(id);
    scheduler.cancel(GAInvalidTaskId);
    const auto expected = succeeds ? GATaskState::Completed : GATaskState::Cancelled;
    return stillBorrowed && stillBusy && task.requests == 1 && scheduler.status(id).state == expected &&
           scheduler.wait(id, 0) && scheduler.waitUntilIdle(0);
}

bool checkWait(IGATaskScheduler& scheduler, GATaskId completed)
{
    using namespace std::chrono_literals;
    BlockingTask task;
    auto started = task.started.get_future();
    auto submit = std::async(std::launch::async, [&] { return scheduler.submit(task); });
    started.wait();
    const auto start = std::chrono::steady_clock::now();
    const bool timedOut = !scheduler.waitUntilIdle(10);
    const bool elapsed = std::chrono::steady_clock::now() - start >= 10ms;
    const bool pollBusy = !scheduler.waitUntilIdle(0);
    const bool previousDone = scheduler.wait(completed, 0);
    const bool unknown = !scheduler.wait(GAInvalidTaskId, 0);
    auto waiter = std::async(std::launch::async, [&] {
        return scheduler.waitUntilIdle(std::numeric_limits<std::uint64_t>::max());
    });
    const bool stillWaiting = waiter.wait_for(10ms) == std::future_status::timeout;
    task.release.set_value();
    const auto id = submit.get();
    return timedOut && elapsed && pollBusy && previousDone && unknown && stillWaiting && waiter.get() &&
           scheduler.wait(id, 0) && scheduler.waitUntilIdle(0);
}

}

int main()
{
    GAWorkflowSession session;
    auto& artifacts = session.artifacts();
    const auto artifact = artifacts.create(GAArtifactType::Mesh, "mesh", "model/mesh", 1);
    auto& scheduler = session.scheduler();
    LookupTask lookup(artifact);
    const auto completed = scheduler.submit(lookup);
    GAWorkflowSession other;
    if (other.artifacts().size() != 0 || other.scheduler().status(completed).id != GAInvalidTaskId)
    {
        return 1;
    }
    const auto foreignLookup = other.scheduler().submit(lookup);
    if (other.scheduler().status(foreignLookup).state != GATaskState::Failed)
    {
        return 1;
    }
    if (!artifacts.remove(artifact))
    {
        return 1;
    }
    const auto missing = scheduler.submit(lookup);
    LookupTask invalid(GAInvalidArtifactId);
    const auto threw = scheduler.submit(invalid);
    const auto success = scheduler.status(completed);
    const auto failure = scheduler.status(missing);
    const auto exception = scheduler.status(threw);
    if (completed == GAInvalidTaskId || success.id != completed || success.state != GATaskState::Completed)
    {
        return 1;
    }
    if (missing == GAInvalidTaskId || missing == threw || failure.id != missing || failure.state != GATaskState::Failed)
    {
        return 1;
    }
    if (threw == GAInvalidTaskId || exception.id != threw || exception.state != GATaskState::Failed)
    {
        return 1;
    }
    if (!checkCancellation(scheduler, false) || !checkCancellation(scheduler, true))
    {
        return 1;
    }
    return scheduler.status(GAInvalidTaskId).id == GAInvalidTaskId && checkWait(scheduler, completed) ? 0 : 1;
}
