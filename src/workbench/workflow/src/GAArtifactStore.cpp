#include "GAArtifactStore.h"

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

class GAArtifactStore::Impl
{
public:
    mutable std::shared_mutex mutex;
    std::unordered_map<GAArtifactId, GAArtifactDescriptor> artifacts;
    GAArtifactId nextId = 1;
};

GAArtifactStore::GAArtifactStore()
    : impl_(std::make_unique<Impl>())
{
}

GAArtifactStore::~GAArtifactStore() = default;

GAArtifactId GAArtifactStore::create(GAArtifactType type, std::string_view name, std::string_view mediaType,
                                     GADataHandle data)
{
    if (type == GAArtifactType::Unknown || data == GAInvalidDataHandle)
    {
        return GAInvalidArtifactId;
    }

    std::unique_lock lock(impl_->mutex);
    const GAArtifactId id = impl_->nextId++;
    // The descriptor stores only a logical link. Ownership of the referenced
    // object remains exclusively in GADataStore.
    impl_->artifacts.emplace(id, GAArtifactDescriptor{id, type, 1, std::string(name), std::string(mediaType), data});
    return id;
}

bool GAArtifactStore::updateData(GAArtifactId artifact, GADataHandle data)
{
    if (artifact == GAInvalidArtifactId || data == GAInvalidDataHandle)
    {
        return false;
    }

    std::unique_lock lock(impl_->mutex);
    const auto found = impl_->artifacts.find(artifact);
    if (found == impl_->artifacts.end())
    {
        return false;
    }
    found->second.data = data;
    ++found->second.version;
    return true;
}

bool GAArtifactStore::descriptor(GAArtifactId artifact, GAArtifactDescriptor& output) const
{
    std::shared_lock lock(impl_->mutex);
    const auto found = impl_->artifacts.find(artifact);
    if (found == impl_->artifacts.end())
    {
        return false;
    }
    output = found->second;
    return true;
}

GADataHandle GAArtifactStore::dataHandle(GAArtifactId artifact) const noexcept
{
    std::shared_lock lock(impl_->mutex);
    const auto found = impl_->artifacts.find(artifact);
    return found == impl_->artifacts.end() ? GAInvalidDataHandle : found->second.data;
}

bool GAArtifactStore::remove(GAArtifactId artifact) noexcept
{
    std::unique_lock lock(impl_->mutex);
    return impl_->artifacts.erase(artifact) != 0;
}

std::size_t GAArtifactStore::size() const noexcept
{
    std::shared_lock lock(impl_->mutex);
    return impl_->artifacts.size();
}

void GAArtifactStore::clear() noexcept
{
    std::unique_lock lock(impl_->mutex);
    impl_->artifacts.clear();
}
