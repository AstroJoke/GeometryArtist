#pragma once

#include <cstdint>
#include <string>

using GAComponentId = std::uint64_t;
inline constexpr GAComponentId GAInvalidComponentId = 0;

enum class GAComponentState : std::uint8_t
{
    Created,
    Initialized,
    Running,
    Paused,
    Stopped,
    Failed
};

/**
 * @brief Stable metadata used to discover a component implementation.
 *
 * typeName is the machine-facing key; displayName is intended for the UI.
 */
struct GAComponentDescriptor
{
    GAComponentId id = GAInvalidComponentId;
    std::string typeName;
    std::string displayName;
    std::uint32_t version = 0;
};
