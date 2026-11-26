#pragma once
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <optional>

namespace ga::session
{
inline constexpr std::size_t kDefaultPendingLimit = 128;
struct SessionPolicy
{
    std::size_t pendingLimit = kDefaultPendingLimit;
};
struct StepTicket
{
    std::uint64_t generation = 0, id = 0;
};
enum class StepStatus
{
    Published,
    Failed,
    Cancelled
};
struct StepCompletion
{
    StepTicket ticket;
    StepStatus status;
    std::exception_ptr error;
};
// Called on the owner or worker thread. Post an event and return without re-entering the queue.
using Wakeup = std::function<void()>;

struct Operation
{
    // prepare and complete run on the owner thread. compute works on private data.
    std::function<void()> prepare, compute;
    std::function<void(const StepCompletion&)> complete;
    // Runs on the compute thread before compute(), using the step's private snapshot.
    std::function<void()> prepareBackground{};
};
// Call session methods on the owner thread, outside operation callbacks.
// Finish publishing one step before preparing the next queued step.
class TaskQueue
{
public:
    explicit TaskQueue(SessionPolicy policy = {}, Wakeup wakeup = {});
    ~TaskQueue();
    TaskQueue(const TaskQueue&) = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;
    std::optional<StepTicket> submit(Operation operation);
    bool poll();
    void wait();   // Wait for the private compute only; publish still requires poll.
    void cancel(); // Invalidate accepted work; an active kernel finishes privately.
    bool pending() const noexcept
    {
        return active_.has_value() || !queue_.empty();
    }
    bool betweenSteps() const noexcept
    {
        return !active_.has_value();
    }
    std::size_t pendingCount() const noexcept
    {
        return queue_.size() + (active_ ? 1 : 0);
    }

private:
    struct Entry
    {
        StepTicket ticket;
        Operation operation;
    };
    void notify() const noexcept;
    Wakeup wakeup_;
    SessionPolicy policy_;
    std::uint64_t generation_ = 1, nextId_ = 1;
    std::deque<Entry> queue_;
    std::optional<Entry> active_;
    std::future<void> result_;
    std::future<void> worker_;
};
} // namespace ga::session
