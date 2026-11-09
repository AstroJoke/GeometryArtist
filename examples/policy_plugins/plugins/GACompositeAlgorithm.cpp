#include "GAAlgorithmPlugin.h"

#include <filesystem>

#if defined(_WIN32)
#define GA_EXAMPLE_EXPORT __declspec(dllexport)
#else
#define GA_EXAMPLE_EXPORT __attribute__((visibility("default")))
#endif

namespace ga::example
{
#if defined(GA_EXAMPLE_BUNDLED)
GAAlgorithmAssembly* createExampleAlgorithm();
#endif

namespace
{
constexpr GAEngineInfo Engine{"example.composed", "Example: composed point operations", 1,
                              static_cast<std::uint32_t>(GAOperation::Scale) |
                                  static_cast<std::uint32_t>(GAOperation::Translate)};
constexpr GAPluginDescriptor Descriptor{GAExamplePluginAbiVersion, Engine};

class CompositeAlgorithm : public GAAlgorithmAssembly
{
public:
    explicit CompositeAlgorithm(const char* pluginDirectory)
        : directory_(pluginDirectory)
    {
    }
    ~CompositeAlgorithm() override
    {
        shutdown();
    }

    bool init() override
    {
        if (isInitialized())
        {
            return true;
        }
        // This module chooses its private implementation. The host knows only example.composed.
        try
        {
#if defined(GA_EXAMPLE_BUNDLED)
            child_.reset(createExampleAlgorithm());
            if (!child_ || !child_->init())
            {
                shutdown();
                return false;
            }
#else
            const auto directory = std::filesystem::path(std::u8string(directory_.begin(), directory_.end()));
            const auto path = (directory / GA_EXAMPLE_CHILD_FILENAME).u8string();
            if (!loader_.load(std::string(path.begin(), path.end()), registry_))
            {
                shutdown();
                return false;
            }
            // An unexpected plugin is rejected before its instance is created.
            child_ = createAlgorithm(registry_, "example.reference");
#endif
            if (!child_ || !child_->supports(GAOperation::Scale) || !child_->supports(GAOperation::Translate))
            {
                shutdown();
                return false;
            }
            return true;
        }
        catch (...)
        {
            shutdown();
            return false;
        }
    }
    void shutdown() noexcept override
    {
#if defined(GA_EXAMPLE_BUNDLED)
        if (child_)
        {
            child_->shutdown();
        }
        child_.reset();
#else
        // Destroy the child and release its factory before removing the private registration.
        child_ = GAAlgorithmInstance{};
        loader_.unload(registry_);
#endif
    }
    bool isInitialized() const noexcept override
    {
        return child_ && child_->isInitialized();
    }
    GAEngineInfo engineInfo() const noexcept override
    {
        return Engine;
    }
    bool supports(GAOperation operation) const noexcept override
    {
        return child_ && child_->supports(operation);
    }
    GAAlgorithmSettings settings() const noexcept override
    {
        return child_ ? child_->settings() : GAAlgorithmSettings{};
    }
    bool setSettings(const GAAlgorithmSettings& settings) noexcept override
    {
        return child_ && child_->setSettings(settings);
    }
    std::string saveSettings() const override
    {
        return child_ ? child_->saveSettings() : std::string{};
    }
    bool restoreSettings(std::string_view text) override
    {
        return child_ && child_->restoreSettings(text);
    }
    void setObserver(IGAAlgorithmObserver& observer) noexcept override
    {
        if (child_)
        {
            child_->setObserver(observer);
        }
    }
    void clearObserver() noexcept override
    {
        if (child_)
        {
            child_->clearObserver();
        }
    }
    GAStatus transform(GAOperation operation, std::span<const GAPoint> input, std::vector<GAPoint>& output) override
    {
        if (!isInitialized())
        {
            return GAStatus::NotInitialized;
        }
        // Execution does not load libraries or query interface types.
        return child_->transform(operation, input, output);
    }

private:
    std::string directory_;
#if defined(GA_EXAMPLE_BUNDLED)
    std::unique_ptr<GAAlgorithmAssembly> child_;
#else
    // Declaration order ensures the private registry outlives its loader.
    GAComponentRegistry registry_;
    GAAlgorithmPluginLoader loader_;
    GAAlgorithmInstance child_;
#endif
};
} // namespace

extern "C" GA_EXAMPLE_EXPORT const GAPluginDescriptor* gaQueryAlgorithmPlugin() noexcept
{
    return &Descriptor;
}
extern "C" GA_EXAMPLE_EXPORT IGAObject* gaCreateAlgorithm(const char* pluginDirectory) noexcept
{
    if (pluginDirectory == nullptr)
    {
        return nullptr;
    }
    try
    {
        return new CompositeAlgorithm(pluginDirectory);
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
} // namespace ga::example
