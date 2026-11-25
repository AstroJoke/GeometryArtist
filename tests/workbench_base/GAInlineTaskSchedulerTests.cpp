#include "GAArtifactStore.h"
#include "GATaskScheduler.h"

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
}

int main()
{
    GAArtifactStore artifacts;
    const auto artifact = artifacts.create(GAArtifactType::Mesh, "mesh", "model/mesh", 1);
    GAInlineTaskScheduler scheduler(artifacts);
    LookupTask lookup(artifact);
    const auto completed = scheduler.submit(lookup);
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
    return scheduler.status(GAInvalidTaskId).id == GAInvalidTaskId ? 0 : 1;
}
