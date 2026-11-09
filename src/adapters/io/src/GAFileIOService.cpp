#include "GAFileIOService.h"

#include "GADataStore.h"
#include "GAFileCodecRegistry.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <unordered_map>

class GAFileIOService::Impl
{
public:
    /**
     * RAII counts an admitted call through completion of its terminal observer callback.
     * A rejected call does not change the counter. This guard does not create a worker thread.
     */
    class OperationGuard
    {
    public:
        explicit OperationGuard(Impl& owner)
            : owner_(owner)
        {
            std::lock_guard lock(owner_.idleMutex);
            if (owner_.initialized && !owner_.closing)
            {
                // A cancellation request applies to the current batch of overlapping calls.
                if (owner_.activeOperations == 0)
                {
                    owner_.cancellation.store(false, std::memory_order_release);
                }
                ++owner_.activeOperations;
                admitted_ = true;
            }
        }

        ~OperationGuard()
        {
            if (admitted_)
            {
                // Update the wait predicate under the same mutex as the waiter.
                {
                    std::lock_guard lock(owner_.idleMutex);
                    --owner_.activeOperations;
                }
                owner_.idleCondition.notify_all();
            }
        }

        OperationGuard(const OperationGuard&) = delete;
        OperationGuard& operator=(const OperationGuard&) = delete;

        explicit operator bool() const noexcept
        {
            return admitted_;
        }

    private:
        Impl& owner_;
        bool admitted_ = false;
    };

    void notify(const GAFileEvent& event) const noexcept
    {
        IGAFileObserver* callback;
        {
            std::lock_guard lock(idleMutex);
            // Snapshot the observer without taking ownership.
            // clearObserver() does not end a callback already using this snapshot.
            callback = observer;
        }
        // Never hold the service lock while invoking external code.
        if (callback != nullptr)
        {
            callback->onEvent(event);
        }
    }

    // Cancellation uses Error + Cancelled: the existing event enum has no Cancelled event.
    GAFileResult finish(const char* path, const GAFileResult& result) const noexcept
    {
        notify({result.status == GAFileStatus::Success ? GAFileEventType::Completed : GAFileEventType::Error, path,
                result.status == GAFileStatus::Success ? 1.0 : 0.0, result.status, result.message});
        return result;
    }

    bool initialized = false;
    bool closing = false;
    GAFileCodecRegistry registry;
    std::unordered_map<std::string, std::string> properties;
    IGAFileObserver* observer = nullptr;
    std::atomic_bool cancellation{false};
    // This counter and lifecycle flags use idleMutex. An atomic counter alone cannot
    // prevent a notification from being lost between predicate checking and waiting.
    std::size_t activeOperations = 0;
    mutable std::mutex idleMutex;
    std::condition_variable idleCondition;
};

GAFileIOService::GAFileIOService()
    : impl_(std::make_unique<Impl>())
{
}

GAFileIOService::~GAFileIOService()
{
    shutdown();
}

bool GAFileIOService::init()
{
    std::lock_guard lock(impl_->idleMutex);
    if (impl_->closing)
    {
        return false;
    }
    if (!impl_->initialized)
    {
        impl_->cancellation.store(false, std::memory_order_release);
        impl_->initialized = true;
    }
    return true;
}

void GAFileIOService::shutdown() noexcept
{
    std::unique_lock lock(impl_->idleMutex);
    if (impl_->closing)
    {
        impl_->idleCondition.wait(lock,
                                  [this]
                                  {
                                      return !impl_->closing;
                                  });
        return;
    }
    if (!impl_->initialized)
    {
        return;
    }
    // Close admission before waiting; a new request must not race registry teardown.
    impl_->closing = true;
    impl_->cancellation.store(true, std::memory_order_release);
    impl_->idleCondition.wait(lock,
                              [this]
                              {
                                  return impl_->activeOperations == 0;
                              });
    impl_->registry.clear();
    impl_->initialized = false;
    impl_->closing = false;
    lock.unlock();
    impl_->idleCondition.notify_all();
}

bool GAFileIOService::isInitialized() const noexcept
{
    std::lock_guard lock(impl_->idleMutex);
    return impl_->initialized && !impl_->closing;
}

const char* GAFileIOService::serviceName() const noexcept
{
    return "GeometryArtist File I/O";
}

std::uint32_t GAFileIOService::serviceVersion() const noexcept
{
    return 1;
}

bool GAFileIOService::setProperty(const char* name, const char* value)
{
    if (name == nullptr || value == nullptr)
    {
        return false;
    }
    impl_->properties.insert_or_assign(name, value);
    return true;
}

const char* GAFileIOService::property(const char* name) const noexcept
{
    if (name == nullptr)
    {
        return nullptr;
    }
    const auto found = impl_->properties.find(name);
    return found == impl_->properties.end() ? nullptr : found->second.c_str();
}

bool GAFileIOService::registerCodec(GAFileCodec* codec)
{
    std::lock_guard lock(impl_->idleMutex);
    return !impl_->closing && impl_->registry.registerCodec(codec);
}

bool GAFileIOService::unregisterCodec(const char* formatId, const char* backendId) noexcept
{
    return impl_->registry.unregisterCodec(formatId, backendId);
}

const GAFileCodec* GAFileIOService::codec(const char* formatId, const char* backendId) const noexcept
{
    return impl_->registry.findById(formatId, backendId);
}

std::size_t GAFileIOService::codecCount() const noexcept
{
    return impl_->registry.size();
}

std::vector<GAFileCodecInfo> GAFileIOService::queryCodecs(const GAFileCodecQuery& query) const
{
    return impl_->registry.queryCodecs(query);
}

void GAFileIOService::setObserver(IGAFileObserver& observer) noexcept
{
    std::lock_guard lock(impl_->idleMutex);
    impl_->observer = &observer;
}

void GAFileIOService::clearObserver() noexcept
{
    std::lock_guard lock(impl_->idleMutex);
    impl_->observer = nullptr;
}

void GAFileIOService::requestCancellation() noexcept
{
    impl_->cancellation.store(true, std::memory_order_release);
}

bool GAFileIOService::cancellationRequested() const noexcept
{
    return impl_->cancellation.load(std::memory_order_acquire);
}

bool GAFileIOService::waitUntilIdle(std::uint64_t timeoutMilliseconds)
{
    std::unique_lock lock(impl_->idleMutex);
    const auto idle = [this]
    {
        // Recheck after spurious wakeups. Idle is an observation, not a ban on future calls.
        return impl_->activeOperations == 0;
    };
    if (timeoutMilliseconds == 0)
    {
        impl_->idleCondition.wait(lock, idle);
        return true;
    }
    return impl_->idleCondition.wait_for(lock, std::chrono::milliseconds(timeoutMilliseconds), idle);
}

GAFileResult GAFileIOService::read(const GAFileReadRequest& request, GADataStore& store)
{
    if (request.filePath == nullptr)
    {
        return {GAFileStatus::InvalidArgument, GAInvalidDataHandle, 0, "Invalid file request"};
    }
    Impl::OperationGuard guard(*impl_);
    if (!guard)
    {
        return {GAFileStatus::InvalidArgument, GAInvalidDataHandle, 0, "File service is not accepting operations"};
    }
    impl_->notify({GAFileEventType::Started, request.filePath, 0.0, GAFileStatus::Success, "Read started"});
    GAFileResult result;
    try
    {
        // Explicit format selection is authoritative; do not fall back on failure.
        const auto selected =
            impl_->registry.resolve({request.backendId, request.formatId, request.filePath, GAFileCapability::Read});
        if (cancellationRequested())
        {
            result = {GAFileStatus::Cancelled, GAInvalidDataHandle, 0, "Read cancelled"};
        }
        else if (selected.status != GAFileStatus::Success)
        {
            result = {selected.status, GAInvalidDataHandle, 0,
                      selected.status == GAFileStatus::SelectionRequired ? "Select a backend and format explicitly"
                      : selected.status == GAFileStatus::Unsupported
                          ? "Selected backend does not support this operation"
                          : "No matching backend/format is registered"};
        }
        else
        {
            result = selected.codec->read(request, store);
        }
    }
    catch (...)
    {
        // Exception text may live in the plugin: return a service-owned static message.
        result = {GAFileStatus::ParseFailed, GAInvalidDataHandle, 0, "File codec read threw an exception"};
    }
    // Every path after Started reaches finish(). The guard stays alive during notification,
    // so waitUntilIdle() also waits for this operation's terminal callback to return.
    return impl_->finish(request.filePath, result);
}

GAFileResult GAFileIOService::write(const GAFileWriteRequest& request, const GADataStore& store)
{
    if (request.filePath == nullptr || request.input == GAInvalidDataHandle)
    {
        return {GAFileStatus::InvalidArgument, GAInvalidDataHandle, 0, "Invalid file request"};
    }
    Impl::OperationGuard guard(*impl_);
    if (!guard)
    {
        return {GAFileStatus::InvalidArgument, GAInvalidDataHandle, 0, "File service is not accepting operations"};
    }
    impl_->notify({GAFileEventType::Started, request.filePath, 0.0, GAFileStatus::Success, "Write started"});
    GAFileResult result;
    try
    {
        // Explicit format selection is authoritative; do not fall back on failure.
        const auto selected = impl_->registry.resolve(
            {request.backendId, request.formatId, request.filePath,
             request.binary ? GAFileCapability::Write | GAFileCapability::Binary : GAFileCapability::Write});
        if (cancellationRequested())
        {
            result = {GAFileStatus::Cancelled, GAInvalidDataHandle, 0, "Write cancelled"};
        }
        else if (selected.status != GAFileStatus::Success)
        {
            result = {selected.status, GAInvalidDataHandle, 0,
                      selected.status == GAFileStatus::SelectionRequired ? "Select a backend and format explicitly"
                      : selected.status == GAFileStatus::Unsupported
                          ? "Selected backend does not support this operation"
                          : "No matching backend/format is registered"};
        }
        else
        {
            // Do not report cancellation after a successful export: the destination may
            // already be committed. Mid-operation cancellation needs codec cooperation.
            result = selected.codec->write(request, store);
        }
    }
    catch (...)
    {
        // Exception text may live in the plugin: return a service-owned static message.
        result = {GAFileStatus::WriteFailed, GAInvalidDataHandle, 0, "File codec write threw an exception"};
    }
    return impl_->finish(request.filePath, result);
}
