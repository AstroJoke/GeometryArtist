#include "GAWorkflowSession.h"

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
    return scheduler.status(GAInvalidTaskId).id == GAInvalidTaskId && checkWait(scheduler, completed) ? 0 : 1;
}
