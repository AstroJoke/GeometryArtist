#include "GADataConverterRegistry.h"

#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

class GADataConverterRegistry::Impl
{
public:
    mutable std::shared_mutex mutex;
    std::unordered_map<std::string, std::shared_ptr<IGADataConverter>> converters;
};

GADataConverterRegistry::GADataConverterRegistry()
    : impl_(std::make_unique<Impl>())
{
}

GADataConverterRegistry::~GADataConverterRegistry() = default;

bool GADataConverterRegistry::registerConverter(std::shared_ptr<IGADataConverter> converter)
{
    if (!converter || converter->converterId() == nullptr)
    {
        return false;
    }
    std::unique_lock lock(impl_->mutex);
    return impl_->converters.emplace(converter->converterId(), std::move(converter)).second;
}

bool GADataConverterRegistry::unregisterConverter(const char* converterId) noexcept
{
    if (converterId == nullptr)
    {
        return false;
    }
    std::unique_lock lock(impl_->mutex);
    return impl_->converters.erase(converterId) != 0;
}

std::shared_ptr<IGADataConverter> GADataConverterRegistry::find(GADataObjectType source,
                                                                GADataObjectType target) const noexcept
{
    std::shared_lock lock(impl_->mutex);
    for (const auto& [id, converter] : impl_->converters)
    {
        (void)id;
        if (converter->sourceType() == source && converter->targetType() == target)
        {
            return converter;
        }
    }
    return nullptr;
}

std::size_t GADataConverterRegistry::size() const noexcept
{
    std::shared_lock lock(impl_->mutex);
    return impl_->converters.size();
}

void GADataConverterRegistry::clear() noexcept
{
    std::unique_lock lock(impl_->mutex);
    impl_->converters.clear();
}
