#pragma once

#include <cstdint>

namespace ga::example
{
enum class GAOperation : std::uint32_t
{
    Scale = 1U << 0,
    Translate = 1U << 1
};

struct GAPoint
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    bool operator==(const GAPoint&) const = default;
};

struct GAAlgorithmSettings
{
    double scale = 1.0;
    GAPoint translation;
};

struct GAEngineInfo
{
    const char* id;
    const char* name;
    std::uint32_t version;
    std::uint32_t capabilities;
};

enum class GAStatus
{
    Success,
    NotInitialized,
    Unsupported,
    InvalidInput
};

enum class GAEvent
{
    Started,
    Completed,
    Failed
};

} // namespace ga::example
