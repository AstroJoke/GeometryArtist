#pragma once

#include "GAFileCodec.h"

#include <cstddef>
#include <cstdint>

/**
 * @brief Binary-interface version shared by the host and file plugins.
 *
 * Increment when exported layouts or entry points change incompatibly.
 */
inline constexpr std::uint32_t GAFilePluginAbiVersion = 2;

/// Static plugin metadata returned by gaQueryFilePlugin().
struct GAFilePluginDescriptor
{
    std::uint32_t abiVersion = GAFilePluginAbiVersion;
    const char* id = nullptr;
    const char* name = nullptr;
    std::uint32_t version = 0;
    std::size_t codecCount = 0;
};

/**
 * Plugins export these C entry points. Returned objects must be destroyed by
 * the same plugin that created them.
 */
using GAQueryFilePluginFunction = const GAFilePluginDescriptor* (*)() noexcept;
using GACreateFileCodecFunction = GAFileCodec* (*)(std::size_t index);
using GADestroyFileCodecFunction = void (*)(GAFileCodec* codec) noexcept;

extern "C" const GAFilePluginDescriptor* gaQueryFilePlugin() noexcept;
extern "C" GAFileCodec* gaCreateFileCodec(std::size_t index);
extern "C" void gaDestroyFileCodec(GAFileCodec* codec) noexcept;
