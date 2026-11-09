#include "GAFileCodecRegistry.h"

#include <map>
#include <mutex>
#include <shared_mutex>
#include <string_view>
#include <utility>

namespace
{
bool supports(GAFileCapability available, GAFileCapability required) noexcept
{
    const auto mask = static_cast<std::uint32_t>(required);
    return (static_cast<std::uint32_t>(available) & mask) == mask;
}

bool matches(const GAFileCodec& codec, const GAFileCodecQuery& query)
{
    const auto& descriptor = codec.descriptor();
    if (query.backendId != nullptr && std::string_view(descriptor.backendId) != query.backendId)
    {
        return false;
    }
    if (query.formatId != nullptr)
    {
        return std::string_view(descriptor.id) == query.formatId;
    }
    if (query.filePath != nullptr)
    {
        const std::string_view path(query.filePath);
        const auto separator = path.find_last_of("/\\");
        const auto dot = path.find_last_of('.');
        return dot != std::string_view::npos && (separator == std::string_view::npos || dot > separator) &&
               codec.acceptsExtension(query.filePath + dot);
    }
    return true;
}
} // namespace

class GAFileCodecRegistry::Impl
{
public:
    // Registered codecs and their descriptors must remain alive and unchanged until unregistered.
    mutable std::shared_mutex mutex;
    std::map<std::pair<std::string, std::string>, GAFileCodec*> codecs;
};

GAFileCodecRegistry::GAFileCodecRegistry()
    : impl_(std::make_unique<Impl>())
{
}

GAFileCodecRegistry::~GAFileCodecRegistry() = default;

bool GAFileCodecRegistry::registerCodec(GAFileCodec* codec)
{
    if (codec == nullptr)
    {
        return false;
    }
    const auto& descriptor = codec->descriptor();
    if (descriptor.id == nullptr || *descriptor.id == '\0' || descriptor.backendId == nullptr ||
        *descriptor.backendId == '\0' || descriptor.backendName == nullptr || *descriptor.backendName == '\0' ||
        (descriptor.extensionCount != 0 && descriptor.extensions == nullptr))
    {
        return false;
    }
    for (std::size_t index = 0; index < descriptor.extensionCount; ++index)
    {
        if (descriptor.extensions[index] == nullptr || *descriptor.extensions[index] == '\0')
        {
            return false;
        }
    }
    std::unique_lock lock(impl_->mutex);
    return impl_->codecs.emplace(std::make_pair(descriptor.backendId, descriptor.id), codec).second;
}

bool GAFileCodecRegistry::unregisterCodec(const char* formatId, const char* backendId) noexcept
{
    if (formatId == nullptr)
    {
        return false;
    }
    std::unique_lock lock(impl_->mutex);
    auto selected = impl_->codecs.end();
    for (auto it = impl_->codecs.begin(); it != impl_->codecs.end(); ++it)
    {
        if (it->first.second == formatId && (backendId == nullptr || it->first.first == backendId))
        {
            if (selected != impl_->codecs.end())
            {
                return false;
            }
            selected = it;
        }
    }
    if (selected == impl_->codecs.end())
    {
        return false;
    }
    impl_->codecs.erase(selected);
    return true;
}

GAFileCodec* GAFileCodecRegistry::findById(const char* formatId, const char* backendId) const noexcept
{
    return formatId == nullptr ? nullptr : resolve({backendId, formatId}).codec;
}

GAFileCodec* GAFileCodecRegistry::findByPath(const char* filePath, const char* backendId) const noexcept
{
    return filePath == nullptr ? nullptr : resolve({backendId, nullptr, filePath}).codec;
}

GAFileCodecRegistry::Match GAFileCodecRegistry::resolve(const GAFileCodecQuery& query) const noexcept
{
    std::shared_lock lock(impl_->mutex);
    GAFileCodec* selected = nullptr;
    for (const auto& [key, codec] : impl_->codecs)
    {
        (void)key;
        if (matches(*codec, query))
        {
            if (selected != nullptr)
            {
                return {nullptr, GAFileStatus::SelectionRequired};
            }
            selected = codec;
        }
    }
    if (selected == nullptr)
    {
        return {};
    }
    if (!supports(selected->descriptor().capabilities, query.required))
    {
        return {nullptr, GAFileStatus::Unsupported};
    }
    return {selected, GAFileStatus::Success};
}

std::vector<GAFileCodecInfo> GAFileCodecRegistry::queryCodecs(const GAFileCodecQuery& query) const
{
    std::shared_lock lock(impl_->mutex);
    std::vector<GAFileCodecInfo> result;
    for (const auto& [key, codec] : impl_->codecs)
    {
        (void)key;
        const auto& descriptor = codec->descriptor();
        if (!matches(*codec, query) || !supports(descriptor.capabilities, query.required))
        {
            continue;
        }
        GAFileCodecInfo info{descriptor.backendId,
                             descriptor.backendName,
                             descriptor.id,
                             descriptor.displayName != nullptr ? descriptor.displayName : "",
                             {},
                             descriptor.capabilities};
        for (std::size_t index = 0; index < descriptor.extensionCount; ++index)
        {
            info.extensions.emplace_back(descriptor.extensions[index]);
        }
        result.push_back(std::move(info));
    }
    return result;
}

std::size_t GAFileCodecRegistry::size() const noexcept
{
    std::shared_lock lock(impl_->mutex);
    return impl_->codecs.size();
}

void GAFileCodecRegistry::clear() noexcept
{
    std::unique_lock lock(impl_->mutex);
    impl_->codecs.clear();
}
