#include "GAFileIOService.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace
{
void check(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

class TestCodec : public GAFileCodec
{
public:
    explicit TestCodec(const char* backendId = "test.default",
                       GAFileCapability capabilities = GAFileCapability::Read | GAFileCapability::Write)
        : info{"test.fake", "Service test", extensions, 1, capabilities, backendId, backendId}
    {
    }
    inline static const char* extensions[] = {"fake"};
    GAFileFormatDescriptor info;
    std::function<void()> action;
    GAFileStatus status = GAFileStatus::Success;
    std::atomic_uint calls{0};

    const GAFileFormatDescriptor& descriptor() const noexcept override
    {
        return info;
    }

    bool acceptsExtension(const char* extension) const noexcept override
    {
        return extension != nullptr && std::string_view(extension) == ".fake";
    }

    GAFileResult read(const GAFileReadRequest&, GADataStore&) override
    {
        return perform();
    }

    GAFileResult write(const GAFileWriteRequest&, const GADataStore&) override
    {
        return perform();
    }

private:
    GAFileResult perform()
    {
        ++calls;
        if (action)
        {
            action();
        }
        return {status, GAInvalidDataHandle, 0, "Test codec result"};
    }
};

struct EventLog : IGAFileObserver
{
    explicit EventLog(GAFileIOService& owner)
        : service(&owner)
    {
    }
    GAFileIOService* service = nullptr;
    bool cancelOnStart = false;
    std::vector<GAFileEvent> events;

    void onEvent(const GAFileEvent& event) noexcept override
    {
        events.push_back(event);
        if (cancelOnStart && event.type == GAFileEventType::Started)
        {
            service->requestCancellation();
        }
    }

    void checkPair(GAFileStatus status) const
    {
        check(events.size() == 2, "Expected exactly one start and terminal event");
        check(events.front().type == GAFileEventType::Started, "Missing start event");
        check(events.back().type ==
                  (status == GAFileStatus::Success ? GAFileEventType::Completed : GAFileEventType::Error),
              "Wrong terminal event");
        check(events.back().status == status, "Terminal status differs from result");
    }
};

// Bounded fake backend; no filesystem/vendor dependency and no arbitrary sleeps.
class Gate
{
public:
    void enter()
    {
        std::unique_lock lock(mutex_);
        entered_ = true;
        condition_.notify_all();
        if (!condition_.wait_for(lock, 3s,
                                 [this]
                                 {
                                     return released_;
                                 }))
        {
            throw std::runtime_error("Test gate timed out");
        }
    }

    void waitForEntry()
    {
        std::unique_lock lock(mutex_);
        check(condition_.wait_for(lock, 3s,
                                  [this]
                                  {
                                      return entered_;
                                  }),
              "Backend did not start");
    }

    void release()
    {
        std::lock_guard lock(mutex_);
        released_ = true;
        condition_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    bool entered_ = false;
    bool released_ = false;
};

void testEvents()
{
    TestCodec codec;
    GAFileIOService service;
    GADataStore store;
    EventLog log(service);
    check(service.init() && service.registerCodec(&codec), "Service setup");
    service.setObserver(log);

    for (bool write : {false, true})
    {
        const auto invoke = [&](const char* format = nullptr)
        {
            log.events.clear();
            return write ? service.write({"test.fake", format, 1}, store) : service.read({"test.fake", format}, store);
        };
        check(invoke().status == GAFileStatus::Success, "Successful dispatch");
        log.checkPair(GAFileStatus::Success);

        check(invoke("unknown").status == GAFileStatus::FormatNotFound, "Unknown format");
        log.checkPair(GAFileStatus::FormatNotFound);

        codec.status = GAFileStatus::Unsupported;
        check(invoke().status == GAFileStatus::Unsupported, "Backend failure");
        log.checkPair(GAFileStatus::Unsupported);
        codec.status = GAFileStatus::Success;

        log.cancelOnStart = true;
        const auto calls = codec.calls.load();
        check(invoke().status == GAFileStatus::Cancelled, "Observer cancellation");
        check(codec.calls == calls, "Cancelled operation reached codec");
        log.checkPair(GAFileStatus::Cancelled);
        log.cancelOnStart = false;

        for (bool standardException : {false, true})
        {
            codec.action = [standardException]
            {
                if (standardException)
                {
                    throw std::runtime_error("Plugin exception");
                }
                throw 7;
            };
            const auto expected = write ? GAFileStatus::WriteFailed : GAFileStatus::ParseFailed;
            check(invoke().status == expected, "Exception status");
            log.checkPair(expected);
            check(service.waitUntilIdle(10), "Exception leaked active-operation count");
        }
        codec.action = {};
    }

    log.events.clear();
    check(service.read({nullptr}, store).status == GAFileStatus::InvalidArgument, "Invalid read");
    check(service.write({"test.fake", nullptr, GAInvalidDataHandle}, store).status == GAFileStatus::InvalidArgument,
          "Invalid write");
    check(log.events.empty(), "Rejected input emitted operation events");
    service.shutdown();
    check(service.read({"test.fake"}, store).status == GAFileStatus::InvalidArgument, "Closed service");
    check(log.events.empty(), "Closed service emitted operation events");
}

void testBackendQueries()
{
    TestCodec reader("test.reader", GAFileCapability::Read | GAFileCapability::Mesh);
    TestCodec writer("test.writer", GAFileCapability::Write | GAFileCapability::Binary | GAFileCapability::Mesh);
    TestCodec duplicate("test.reader");
    GAFileIOService service;
    GADataStore store;
    check(service.init(), "Query service init");
    check(service.queryCodecs().empty(), "Unregistered backends were advertised");
    check(service.registerCodec(&writer) && service.registerCodec(&reader), "Same format across two backends");
    check(!service.registerCodec(&duplicate), "Duplicate backend/format pair accepted");
    TestCodec invalid(nullptr);
    check(!service.registerCodec(&invalid), "Missing backend ID accepted");
    invalid.info.backendId = "test.invalid";
    invalid.info.backendName = "Invalid";
    invalid.info.extensions = nullptr;
    check(!service.registerCodec(&invalid), "Invalid extension array accepted");

    // Query through the public policy, not through the concrete registry.
    const GAFileCodecPolicy& capabilities = service;
    const auto all = capabilities.queryCodecs();
    check(all.size() == 2 && all[0].backendId == "test.reader" && all[1].backendId == "test.writer",
          "Backend catalogue must be stable and retain both implementations");
    check(all[0].formatId == "test.fake" && all[0].extensions == std::vector<std::string>{"fake"},
          "Catalogue metadata");
    check(capabilities.queryCodecs({.required = GAFileCapability::Read}).size() == 1, "Read capability filter");
    check(capabilities.queryCodecs({.required = GAFileCapability::Write | GAFileCapability::Binary}).size() == 1,
          "Combined capability filter");
    check(capabilities.queryCodecs({.required = GAFileCapability::Read | GAFileCapability::Write}).empty(),
          "Required bits must all belong to one implementation");
    check(capabilities.queryCodecs({.backendId = "test.reader", .required = GAFileCapability::Binary}).empty(),
          "Backend filter must not fall back to another provider");
    check(capabilities.queryCodecs({.filePath = "input.fake"}).size() == 2, "Extension lookup across providers");
    check(capabilities.queryCodecs({.filePath = "directory.fake/input"}).empty(), "Directory dot is not a format");
    check(capabilities.queryCodecs({.backendId = "missing"}).empty(), "Unknown backend query");
    check(capabilities.queryCodecs({.formatId = "missing", .filePath = "input.fake"}).empty(),
          "Explicit unknown format must not fall back to extension");
    check(capabilities.queryCodecs({.formatId = "test.fake", .filePath = "input.unknown"}).size() == 2,
          "Explicit format should take precedence");
    check(service.codec("test.fake") == nullptr && service.codec("test.fake", "test.reader") == &reader,
          "Unqualified lookup must reject ambiguity");
    check(!service.unregisterCodec("test.fake") && service.codecCount() == 2, "Ambiguous removal changed registry");

    EventLog log(service);
    service.setObserver(log);
    check(service.read({"input.fake"}, store).status == GAFileStatus::SelectionRequired,
          "Ambiguous read silently selected a backend");
    log.checkPair(GAFileStatus::SelectionRequired);
    check(reader.calls == 0 && writer.calls == 0, "Ambiguous read reached backend");
    log.events.clear();
    check(service.write({.filePath = "output.fake", .input = 1}, store).status == GAFileStatus::SelectionRequired,
          "Ambiguous write silently selected a backend");
    log.checkPair(GAFileStatus::SelectionRequired);
    service.clearObserver();

    check(service.read({.filePath = "input.fake", .backendId = "test.reader"}, store).status == GAFileStatus::Success,
          "Explicit reader dispatch");
    check(service.write({.filePath = "output.fake", .input = 1, .binary = true, .backendId = "test.writer"}, store)
                  .status == GAFileStatus::Success,
          "Explicit binary writer dispatch");
    const auto readCalls = reader.calls.load();
    const auto writeCalls = writer.calls.load();
    check(service.read({.filePath = "input.fake", .backendId = "test.writer"}, store).status ==
              GAFileStatus::Unsupported,
          "Unsupported read was dispatched");
    check(service.write({.filePath = "output.fake", .input = 1, .backendId = "test.reader"}, store).status ==
              GAFileStatus::Unsupported,
          "Unsupported write was dispatched");
    check(service.read({.filePath = "input.fake", .backendId = "missing"}, store).status ==
              GAFileStatus::FormatNotFound,
          "Unknown backend fell back to a registered provider");
    check(reader.calls == readCalls && writer.calls == writeCalls, "Rejected requests reached backend");
    check(service.unregisterCodec("test.fake", "test.writer"), "Qualified removal failed");
    check(service.read({"input.fake"}, store).status == GAFileStatus::Success, "Unique backend compatibility");
    check(service.queryCodecs().size() == 1 && all.size() == 2, "Query snapshot changed after unregister");
    service.shutdown();
    check(service.queryCodecs().empty(), "Shutdown retained query entries");

    std::vector<GAFileCodecInfo> retained;
    {
        std::string name = "temporary.backend";
        TestCodec temporary(name.c_str());
        check(service.init() && service.registerCodec(&temporary), "Temporary backend registration");
        retained = service.queryCodecs();
        service.shutdown();
        name.assign(name.size(), 'x');
    }
    check(retained.size() == 1 && retained.front().backendId == "temporary.backend" &&
              retained.front().backendName == "temporary.backend",
          "Query metadata borrows destroyed backend strings");
}

void testObserverLifetime()
{
    struct BlockingObserver : IGAFileObserver
    {
        Gate gate;
        int started = 0;
        int completed = 0;
        void onEvent(const GAFileEvent& event) noexcept override
        {
            if (event.type == GAFileEventType::Started)
            {
                ++started;
                gate.enter();
            }
            else
            {
                ++completed;
            }
        }
    } observer;
    TestCodec codec;
    GAFileIOService service;
    GADataStore store;
    check(service.init() && service.registerCodec(&codec), "Observer setup");
    service.setObserver(observer);
    auto operation = std::async(std::launch::async,
                                [&]
                                {
                                    return service.read({"test.fake"}, store);
                                });
    observer.gate.waitForEntry();
    service.clearObserver();
    const bool stillRunning = !service.waitUntilIdle(1);
    observer.gate.release();
    check(operation.get().status == GAFileStatus::Success, "Cleared observer interrupted operation");
    check(stillRunning && service.waitUntilIdle(1000), "Idle wait did not include active callback");
    check(observer.started == 1 && observer.completed == 0, "Cleared observer received terminal event");

    EventLog first(service);
    EventLog replacement(service);
    service.setObserver(first);
    service.setObserver(replacement);
    check(service.read({"test.fake"}, store).status == GAFileStatus::Success, "Replacement observer dispatch");
    check(first.events.empty(), "Replaced observer received events");
    replacement.checkPair(GAFileStatus::Success);
    service.clearObserver();
    check(service.read({"test.fake"}, store).status == GAFileStatus::Success && replacement.events.size() == 2,
          "Cleared replacement received events");
}

void testWaitAndCancellation()
{
    TestCodec codec;
    GAFileIOService service;
    GADataStore store;
    check(service.init() && service.registerCodec(&codec), "Wait setup");

    for (int iteration = 0; iteration < 64; ++iteration)
    {
        Gate gate;
        codec.action = [&gate]
        {
            gate.enter();
        };
        auto operation = std::async(std::launch::async,
                                    [&]
                                    {
                                        return service.read({"test.fake"}, store);
                                    });
        gate.waitForEntry();
        check(!service.waitUntilIdle(1), "Reported idle while codec is running");
        service.requestCancellation();
        check(service.init() && service.cancellationRequested(), "Repeated init cleared cancellation");
        check(service.read({"test.fake"}, store).status == GAFileStatus::Cancelled,
              "Overlapping call cleared cancellation");

        std::promise<void> ready;
        auto waiter = std::async(std::launch::async,
                                 [&]
                                 {
                                     ready.set_value();
                                     return service.waitUntilIdle(iteration % 2 == 0 ? 0 : 1000);
                                 });
        ready.get_future().wait();
        gate.release();
        check(operation.get().status == GAFileStatus::Success,
              "Service must not turn completed non-cooperative work into cancellation");
        check(waiter.wait_for(2s) == std::future_status::ready, "Waiter lost completion notification");
        check(waiter.get(), "Wait timed out after completion");
    }
    codec.action = {};
    check(service.read({"test.fake"}, store).status == GAFileStatus::Success,
          "New idle-to-active batch did not reset cancellation");
}

void testShutdownAdmission()
{
    Gate gate;
    TestCodec codec;
    codec.action = [&gate]
    {
        gate.enter();
    };
    GAFileIOService service;
    GADataStore store;
    check(service.init() && service.registerCodec(&codec), "Shutdown setup");
    auto operation = std::async(std::launch::async,
                                [&]
                                {
                                    return service.read({"test.fake"}, store);
                                });
    gate.waitForEntry();
    auto shutdown = std::async(std::launch::async,
                               [&]
                               {
                                   service.shutdown();
                               });
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (service.isInitialized() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::yield();
    }
    check(!service.isInitialized(), "Shutdown did not close admission");
    check(!service.init(), "Init reopened a shutting-down service");
    check(!service.registerCodec(&codec), "Registration accepted during shutdown");
    check(service.read({"test.fake"}, store).status == GAFileStatus::InvalidArgument,
          "Shutdown admitted a new operation");
    check(service.cancellationRequested(), "Rejected request reset shutdown cancellation");
    gate.release();
    check(operation.get().status == GAFileStatus::Success, "Existing operation failed");
    check(shutdown.wait_for(2s) == std::future_status::ready, "Shutdown did not finish");
    shutdown.get();
    check(service.codecCount() == 0, "Shutdown retained registered codecs");
    check(service.init() && service.registerCodec(&codec), "Service could not restart");
}

} // namespace

int main()
{
    try
    {
        testEvents();
        testBackendQueries();
        testObserverLifetime();
        testWaitAndCancellation();
        testShutdownAdmission();
        std::cout << "File service lifecycle and event tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
