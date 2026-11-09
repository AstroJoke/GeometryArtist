#include "GAAlgorithmPlugin.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iterator>
#include <limits>
#include <locale>
#include <sstream>

#if defined(_WIN32)
#define GA_EXAMPLE_EXPORT __declspec(dllexport)
#else
#define GA_EXAMPLE_EXPORT __attribute__((visibility("default")))
#endif

namespace ga::example
{
namespace
{
#if defined(GA_EXAMPLE_STANDARD)
constexpr GAEngineInfo Engine{"example.standard", "Example: standard algorithm", 1,
                              static_cast<std::uint32_t>(GAOperation::Scale)};
#else
constexpr GAEngineInfo Engine{"example.reference", "Example: reference loop", 1,
                              static_cast<std::uint32_t>(GAOperation::Scale) |
                                  static_cast<std::uint32_t>(GAOperation::Translate)};
#endif
constexpr GAPluginDescriptor Descriptor{GAExamplePluginAbiVersion, Engine};

bool finite(GAPoint point)
{
    return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
}

class ExampleAlgorithm : public GAAlgorithmAssembly
{
public:
    bool init() override
    {
        initialized_ = true;
        return true;
    }
    void shutdown() noexcept override
    {
        initialized_ = false;
        clearObserver();
    }
    bool isInitialized() const noexcept override
    {
        return initialized_;
    }
    GAEngineInfo engineInfo() const noexcept override
    {
        return Engine;
    }
    bool supports(GAOperation operation) const noexcept override
    {
        return (operation == GAOperation::Scale || operation == GAOperation::Translate) &&
               (Engine.capabilities & static_cast<std::uint32_t>(operation)) != 0;
    }
    GAAlgorithmSettings settings() const noexcept override
    {
        return settings_;
    }
    bool setSettings(const GAAlgorithmSettings& settings) noexcept override
    {
        if (!std::isfinite(settings.scale) || !finite(settings.translation))
        {
            return false;
        }
        settings_ = settings;
        return true;
    }
    std::string saveSettings() const override
    {
        std::ostringstream stream;
        stream.imbue(std::locale::classic());
        stream << std::setprecision(std::numeric_limits<double>::max_digits10) << "ga.example.settings.v1 "
               << settings_.scale << ' ' << settings_.translation.x << ' ' << settings_.translation.y << ' '
               << settings_.translation.z;
        return stream.str();
    }
    bool restoreSettings(std::string_view text) override
    {
        std::istringstream stream{std::string(text)};
        stream.imbue(std::locale::classic());
        std::string version;
        GAAlgorithmSettings settings;
        if (!(stream >> version >> settings.scale >> settings.translation.x >> settings.translation.y >>
              settings.translation.z) ||
            version != "ga.example.settings.v1")
        {
            return false;
        }
        stream >> std::ws;
        return stream.eof() && setSettings(settings);
    }
    void setObserver(IGAAlgorithmObserver& observer) noexcept override
    {
        observer_ = &observer;
    }
    void clearObserver() noexcept override
    {
        observer_ = nullptr;
    }

    GAStatus transform(GAOperation operation, std::span<const GAPoint> input, std::vector<GAPoint>& output) override
    {
        if (!initialized_)
        {
            return GAStatus::NotInitialized;
        }
        if (!supports(operation))
        {
            return GAStatus::Unsupported;
        }
        if (!std::all_of(input.begin(), input.end(), finite))
        {
            return GAStatus::InvalidInput;
        }
        // Allocate before notification; output remains unchanged on allocation failure.
        std::vector<GAPoint> transformed(input.size());
        notify(GAEvent::Started, GAStatus::Success);
        const auto transformPoint = [&](GAPoint p)
        {
            return operation == GAOperation::Scale
                       ? GAPoint{p.x * settings_.scale, p.y * settings_.scale, p.z * settings_.scale}
                       : GAPoint{p.x + settings_.translation.x, p.y + settings_.translation.y,
                                 p.z + settings_.translation.z};
        };
#if defined(GA_EXAMPLE_STANDARD)
        std::transform(input.begin(), input.end(), transformed.begin(), transformPoint);
#else
        for (std::size_t index = 0; index < input.size(); ++index)
        {
            transformed[index] = transformPoint(input[index]);
        }
#endif
        if (!std::all_of(transformed.begin(), transformed.end(), finite))
        {
            notify(GAEvent::Failed, GAStatus::InvalidInput);
            return GAStatus::InvalidInput;
        }
        output = std::move(transformed);
        notify(GAEvent::Completed, GAStatus::Success);
        return GAStatus::Success;
    }

private:
    void notify(GAEvent event, GAStatus status) noexcept
    {
        if (observer_ != nullptr)
        {
            observer_->onEvent(event, status);
        }
    }
    bool initialized_ = false;
    GAAlgorithmSettings settings_;
    IGAAlgorithmObserver* observer_ = nullptr;
};
} // namespace

GAAlgorithmAssembly* createExampleAlgorithm()
{
    return new ExampleAlgorithm;
}

#if !defined(GA_EXAMPLE_EMBEDDED)
extern "C" GA_EXAMPLE_EXPORT const GAPluginDescriptor* gaQueryAlgorithmPlugin() noexcept
{
    return &Descriptor;
}
extern "C" GA_EXAMPLE_EXPORT IGAObject* gaCreateAlgorithm(const char*) noexcept
{
    try
    {
        return createExampleAlgorithm();
    }
    catch (...)
    {
        return nullptr;
    }
}
extern "C" GA_EXAMPLE_EXPORT void gaDestroyAlgorithm(IGAObject* object) noexcept
{
    delete object;
}
#endif
} // namespace ga::example
