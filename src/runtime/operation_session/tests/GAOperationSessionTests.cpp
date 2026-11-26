#include <GATaskQueue.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <latch>
#include <semaphore>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>
using namespace ga::session;
void require(bool ok)
{
    if (!ok)
        throw std::runtime_error("Session contract failed");
}
void backgroundPreparation()
{
    const auto owner = std::this_thread::get_id();
    std::thread::id preparedOn;
    bool preparedOnOwner = false;
    bool published = false;
    TaskQueue queue;
    Operation step;
    step.prepare = [&] { preparedOnOwner = std::this_thread::get_id() == owner; };
    step.prepareBackground = [&]
    {
        require(preparedOnOwner);
        preparedOn = std::this_thread::get_id();
        require(preparedOn != owner);
    };
    step.compute = [&] { require(std::this_thread::get_id() == preparedOn); };
    step.complete = [&](const StepCompletion& result)
    {
        require(std::this_thread::get_id() == owner && result.status == StepStatus::Published);
        const auto& t = result.timeline;
        require(t.prepareStarted && t.prepared && t.computeStarted && t.backgroundPrepared && t.computed && t.publishStarted);
        require(t.submitted <= *t.prepareStarted && *t.prepareStarted <= *t.prepared &&
                *t.prepared <= *t.computeStarted && *t.computeStarted <= *t.backgroundPrepared &&
                *t.backgroundPrepared <= *t.computed && *t.computed <= *t.publishStarted);
        published = true;
    };
    require(queue.submit(std::move(step)).has_value());
    require(queue.poll());
    queue.wait();
    require(!published && queue.poll() && published);
}

void eventDrivenQueue()
{
    using namespace std::chrono_literals;
    std::counting_semaphore<> notifications(0);
    TaskQueue queue({}, [&] { notifications.release(); });
    std::vector<StepStatus> outcomes;
    std::vector<StepTimeline> timelines;
    auto complete = [&](const StepCompletion& result)
    {
        outcomes.push_back(result.status);
        timelines.push_back(result.timeline);
    };
    require(queue.submit({[] { throw std::runtime_error("prepare"); }, [] {}, complete}).has_value());
    require(queue.submit({{}, [] { throw std::runtime_error("compute"); }, complete}).has_value());
    bool ranAfterFailedPreparation = false;
    require(queue.submit({{}, [&] { ranAfterFailedPreparation = true; }, complete,
                          [] { throw std::runtime_error("background prepare"); }}).has_value());
    require(queue.submit({{}, [] {}, complete}).has_value());
    while (queue.pending())
    {
        require(notifications.try_acquire_for(5s));
        while (notifications.try_acquire()) {}
        require(queue.poll());
    }
    require(!ranAfterFailedPreparation);
    require(outcomes == std::vector<StepStatus>{StepStatus::Failed, StepStatus::Failed, StepStatus::Failed,
                                                StepStatus::Published});
    const auto& ownerFailure = timelines[0];
    require(ownerFailure.prepareStarted && !ownerFailure.prepared && !ownerFailure.computeStarted &&
            ownerFailure.publishStarted);
    const auto& computeFailure = timelines[1];
    require(computeFailure.backgroundPrepared && computeFailure.computed && computeFailure.publishStarted);
    const auto& backgroundFailure = timelines[2];
    require(backgroundFailure.computeStarted && !backgroundFailure.backgroundPrepared &&
            !backgroundFailure.computed && backgroundFailure.publishStarted);
}

int main()
{
    backgroundPreparation();
    eventDrivenQueue();
    TaskQueue session({2});
    std::latch entered(1), release(1);
    const auto owner = std::this_thread::get_id();
    std::vector<int> published;
    std::atomic<bool> secondStarted = false;
    require(session
                .submit({{},
                         [&]
                         {
                             require(std::this_thread::get_id() != owner);
                             entered.count_down();
                             release.wait();
                         },
                         [&](auto done)
                         {
                             require(std::this_thread::get_id() == owner);
                             require(done.status == StepStatus::Published);
                             published.push_back(1);
                         }})
                .has_value());
    require(session
                .submit({[&]
                         {
                             require(published.size() == 1);
                         },
                         [&]
                         {
                             secondStarted = true;
                         },
                         [&](auto done)
                         {
                             require(done.status == StepStatus::Published);
                             published.push_back(2);
                         }})
                .has_value());
    require(!session.submit({{}, [] {}, [](auto) {}}));
    require(session.pending() && session.betweenSteps());
    session.poll();
    entered.wait();
    require(!session.poll());
    require(!secondStarted);
    require(!session.betweenSteps());
    release.count_down();
    session.wait();
    require(published.empty());
    require(!session.betweenSteps()); // Completion must be published before maintenance.
    session.poll();
    require(published.size() == 1 && !secondStarted);
    require(session.pending() && session.betweenSteps());
    session.poll();
    session.wait();
    session.poll();
    require(!session.pending() && published.size() == 2);
    std::vector<StepStatus> statuses;
    session.submit({{},
                    []
                    {
                        throw std::runtime_error("kernel failure");
                    },
                    [&](auto done)
                    {
                        require(bool(done.error));
                        statuses.push_back(done.status);
                    }});
    session.poll();
    session.wait();
    session.poll();
    require(statuses.back() == StepStatus::Failed);
    std::latch busy(1), resume(1);
    session.submit({{},
                    [&]
                    {
                        busy.count_down();
                        resume.wait();
                    },
                    [&](auto done)
                    {
                        statuses.push_back(done.status);
                    }});
    session.poll();
    busy.wait();
    session.submit({{},
                    []
                    {
                        throw std::runtime_error("cancelled work ran");
                    },
                    [&](auto done)
                    {
                        require(!done.timeline.prepareStarted && !done.timeline.computeStarted &&
                                !done.timeline.computed && done.timeline.publishStarted);
                        statuses.push_back(done.status);
                    }});
    session.cancel();
    require(!session.betweenSteps());
    resume.count_down();
    session.wait();
    require(!session.betweenSteps());
    session.poll();
    require(session.betweenSteps());
    require(statuses.size() == 3 && statuses[1] == StepStatus::Cancelled && statuses[2] == StepStatus::Cancelled);
    const auto ticket = session.submit({{},
                                        [] {},
                                        [](auto done)
                                        {
                                            require(done.status == StepStatus::Published);
                                        }});
    require(ticket && ticket->generation == 2);
    session.poll();
    session.wait();
    session.poll();
    std::cout << "Ordered publication, bounded admission, failure, cancellation and thread boundaries passed\n";
}
