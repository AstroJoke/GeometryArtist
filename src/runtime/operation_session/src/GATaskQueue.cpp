#include <GATaskQueue.h>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <utility>
namespace ga::session
{
TaskQueue::TaskQueue(SessionPolicy policy, Wakeup wakeup)
    : wakeup_(std::move(wakeup))
    , policy_(policy)
{
    if (!policy.pendingLimit)
        throw std::invalid_argument("Session capacity must be positive");
}

TaskQueue::~TaskQueue()
{
    if (worker_.valid())
        worker_.wait();
}

void TaskQueue::notify() const noexcept
{
    try
    {
        if (wakeup_)
            wakeup_();
    }
    catch (...)
    {
        // A notification failure does not change the operation result.
    }
}

std::optional<StepTicket> TaskQueue::submit(Operation operation)
{
    if (!operation.compute || !operation.complete)
        throw std::invalid_argument("Incomplete operation");
    if (pendingCount() >= policy_.pendingLimit)
        return std::nullopt;
    if (nextId_ == std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("Step ID exhausted");
    const StepTicket ticket{generation_, nextId_++};
    queue_.push_back({ticket, std::move(operation), {StepClock::now()}});
    notify();
    return ticket;
}
bool TaskQueue::poll()
{
    if (active_)
    {
        if (result_.wait_for(std::chrono::seconds::zero()) != std::future_status::ready)
            return false;
        std::exception_ptr error;
        try
        {
            result_.get();
        }
        catch (...)
        {
            error = std::current_exception();
        }
        worker_.get();
        auto entry = std::move(*active_);
        active_.reset();
        auto status = error ? StepStatus::Failed : StepStatus::Published;
        if (entry.ticket.generation != generation_)
            status = StepStatus::Cancelled;
        entry.timeline.publishStarted = StepClock::now();
        entry.operation.complete({entry.ticket, status, error, entry.timeline});
        if (!queue_.empty())
            notify();
        return true;
    }
    if (queue_.empty())
        return false;
    active_ = std::move(queue_.front());
    queue_.pop_front();
    active_->timeline.prepareStarted = StepClock::now();
    try
    {
        if (active_->operation.prepare)
            active_->operation.prepare();
        active_->timeline.prepared = StepClock::now();
        auto& operation = active_->operation;
        // The worker owns these timestamps until poll() joins it and consumes the entry.
        std::packaged_task<void()> run([&timeline = active_->timeline, prepare = std::move(operation.prepareBackground),
                                        compute = std::move(operation.compute)]
        {
            timeline.computeStarted = StepClock::now();
            if (prepare)
                prepare();
            timeline.backgroundPrepared = StepClock::now();
            try
            {
                compute();
            }
            catch (...)
            {
                timeline.computed = StepClock::now();
                throw;
            }
            timeline.computed = StepClock::now();
        });
        result_ = run.get_future();
        worker_ = std::async(std::launch::async, [this, run = std::move(run)]() mutable
        {
            run(); // Makes the result or exception available before waking the owner.
            notify();
        });
    }
    catch (...)
    {
        auto entry = std::move(*active_);
        active_.reset();
        entry.timeline.publishStarted = StepClock::now();
        entry.operation.complete({entry.ticket, StepStatus::Failed, std::current_exception(), entry.timeline});
        if (!queue_.empty())
            notify();
    }
    return true;
}
void TaskQueue::wait()
{
    if (worker_.valid())
        worker_.wait();
}
void TaskQueue::cancel()
{
    if (generation_ == std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("Session generation exhausted");
    ++generation_;
    auto cancelled = std::move(queue_);
    queue_.clear();
    for (auto& entry : cancelled)
    {
        entry.timeline.publishStarted = StepClock::now();
        entry.operation.complete({entry.ticket, StepStatus::Cancelled, {}, entry.timeline});
    }
    if (!cancelled.empty())
        notify();
}
} // namespace ga::session
