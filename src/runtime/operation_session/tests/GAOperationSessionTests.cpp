#include <GATaskQueue.h>
#include <atomic>
#include <iostream>
#include <latch>
#include <stdexcept>
#include <thread>
#include <vector>
using namespace ga::session;
void require(bool ok)
{
    if (!ok)
        throw std::runtime_error("Session contract failed");
}
int main()
{
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
