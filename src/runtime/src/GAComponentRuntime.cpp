#include "GAComponentRuntime.h"

#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

class GAComponentRegistry::Impl
{
public:
    mutable std::shared_mutex mutex;
    std::unordered_map<std::string, std::shared_ptr<IGAComponentFactory>> factories;
};

GAComponentRegistry::GAComponentRegistry()
    : impl_(std::make_unique<Impl>())
{
}

GAComponentRegistry::~GAComponentRegistry() = default;

bool GAComponentRegistry::registerFactory(std::shared_ptr<IGAComponentFactory> factory)
{
    if (!factory)
    {
        return false;
    }

    const GAComponentDescriptor descriptor = factory->descriptor();
    if (descriptor.typeName.empty())
    {
        return false;
    }

    std::unique_lock lock(impl_->mutex);
    return impl_->factories.emplace(descriptor.typeName, std::move(factory)).second;
}

bool GAComponentRegistry::unregisterFactory(std::string_view typeName)
{
    std::unique_lock lock(impl_->mutex);
    return impl_->factories.erase(std::string(typeName)) != 0;
}

std::shared_ptr<IGAComponentFactory> GAComponentRegistry::find(std::string_view typeName) const
{
    std::shared_lock lock(impl_->mutex);
    const auto found = impl_->factories.find(std::string(typeName));
    return found == impl_->factories.end() ? nullptr : found->second;
}

std::size_t GAComponentRegistry::size() const noexcept
{
    std::shared_lock lock(impl_->mutex);
    return impl_->factories.size();
}

void GAComponentRegistry::clear() noexcept
{
    std::unique_lock lock(impl_->mutex);
    impl_->factories.clear();
}
