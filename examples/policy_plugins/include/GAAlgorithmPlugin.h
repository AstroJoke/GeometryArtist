#pragma once

#include "GAAlgorithmAssembly.h"
#include "GAComponentRuntime.h"

#include <memory>
#include <string>

namespace ga::example
{
inline constexpr std::uint32_t GAExamplePluginAbiVersion = 3;

struct GAPluginDescriptor
{
    std::uint32_t abiVersion;
    GAEngineInfo engine;
};

// C entry points create C++ interface objects; host and plugins use a matching C++ ABI.
using GAQueryPlugin = const GAPluginDescriptor* (*)() noexcept;
// The directory locates private dependencies; it does not prescribe which dependencies to use.
// The pointer is borrowed during creation. The plugin copies it if needed afterwards.
using GACreateAlgorithm = IGAObject* (*)(const char* pluginDirectory) noexcept;
using GADestroyAlgorithm = void (*)(IGAObject*) noexcept;

class GAAlgorithmPluginLoader : public IGAComponentPluginLoader
{
public:
    GAAlgorithmPluginLoader();
    ~GAAlgorithmPluginLoader() override;
    GAAlgorithmPluginLoader(const GAAlgorithmPluginLoader&) = delete;
    GAAlgorithmPluginLoader& operator=(const GAAlgorithmPluginLoader&) = delete;

    // One library per loader. The registry must outlive this loader.
    bool load(std::string_view path, IGAComponentRegistry& registry) override;
    void unload(IGAComponentRegistry& registry) noexcept override;
    [[nodiscard]] const std::string& error() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

struct GAAlgorithmDeleter
{
    // Retains the factory and therefore its library until object destruction finishes.
    std::shared_ptr<IGAComponentFactory> factory;
    void operator()(GAAlgorithmAssembly* object) const noexcept;
};
using GAAlgorithmInstance = std::unique_ptr<GAAlgorithmAssembly, GAAlgorithmDeleter>;
[[nodiscard]] GAAlgorithmInstance createAlgorithm(IGAComponentRegistry& registry, std::string_view engineId);
} // namespace ga::example
