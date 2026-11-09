#pragma once

#include "GAFilePlugin.h"

#include <cstddef>
#include <memory>
#include <string>

/**
 * @brief Owns one dynamic file plugin and all codecs it creates.
 *
 * Destruction releases codecs before unloading the library. Returned codecs and
 * descriptors are borrowed and become invalid after unload().
 */
class GAFilePluginLoader
{
public:
    GAFilePluginLoader();
    ~GAFilePluginLoader();

    GAFilePluginLoader(const GAFilePluginLoader&) = delete;
    GAFilePluginLoader& operator=(const GAFilePluginLoader&) = delete;
    GAFilePluginLoader(GAFilePluginLoader&&) noexcept;
    GAFilePluginLoader& operator=(GAFilePluginLoader&&) noexcept;

    /// Loads one library, validates its ABI, and creates all advertised codecs.
    [[nodiscard]] bool load(const std::string& filePath);
    void unload() noexcept;

    [[nodiscard]] bool isLoaded() const noexcept;
    [[nodiscard]] const GAFilePluginDescriptor* descriptor() const noexcept;
    [[nodiscard]] std::size_t codecCount() const noexcept;
    [[nodiscard]] GAFileCodec* codec(std::size_t index) noexcept;
    [[nodiscard]] const std::string& errorString() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
