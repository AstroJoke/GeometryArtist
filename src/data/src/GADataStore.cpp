#include "GADataStore.h"

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

class GADataStore::Impl
{
public:
    // The map owns shared control blocks so an outstanding access lease can
    // outlive removal from the map.
    mutable std::shared_mutex mutex;
    std::unordered_map<GADataHandle, std::shared_ptr<GADataObject>> objects;
    GADataHandle nextHandle = 1;
};

GADataStore::GADataStore()
    : impl_(std::make_unique<Impl>())
{
}

GADataStore::~GADataStore() = default;

GADataHandle GADataStore::registe(std::unique_ptr<GADataObject> object)
{
    if (!object)
    {
        return GAInvalidDataHandle;
    }

    std::unique_lock lock(impl_->mutex);
    const GADataHandle handle = impl_->nextHandle++;
    // At this boundary the unique owner is deliberately converted into shared
    // lifetime management for concurrent readers.
    object->assignId(handle);
    impl_->objects.emplace(handle, std::shared_ptr<GADataObject>(std::move(object)));
    return handle;
}

std::shared_ptr<const GADataObject> GADataStore::access(GADataHandle handle) const noexcept
{
    std::shared_lock lock(impl_->mutex);
    const auto found = impl_->objects.find(handle);
    return found == impl_->objects.end() ? nullptr : found->second;
}

std::shared_ptr<GADataObject> GADataStore::accessMutable(GADataHandle handle) noexcept
{
    std::shared_lock lock(impl_->mutex);
    const auto found = impl_->objects.find(handle);
    return found == impl_->objects.end() ? nullptr : found->second;
}

bool GADataStore::replace(GADataHandle handle, std::unique_ptr<GADataObject> object)
{
    if (GAInvalidDataHandle == handle || nullptr == object)
    {
        return false;
    }

    std::unique_lock lock(impl_->mutex);
    const auto found = impl_->objects.find(handle);
    if (found == impl_->objects.end())
    {
        return false;
    }
    object->assignId(handle);
    // Existing shared_ptr leases continue to refer to the previous revision;
    // only future lookups observe the replacement.
    found->second = std::shared_ptr<GADataObject>(std::move(object));
    return true;
}

bool GADataStore::release(GADataHandle handle) noexcept
{
    std::unique_lock lock(impl_->mutex);
    return impl_->objects.erase(handle) != 0;
}

bool GADataStore::contains(GADataHandle handle) const noexcept
{
    std::shared_lock lock(impl_->mutex);
    return impl_->objects.contains(handle);
}

std::size_t GADataStore::size() const noexcept
{
    std::shared_lock lock(impl_->mutex);
    return impl_->objects.size();
}

void GADataStore::clear() noexcept
{
    std::unique_lock lock(impl_->mutex);
    impl_->objects.clear();
}
