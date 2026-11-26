#pragma once

#include "GADataTypes.h"

#include <cstdint>
#include <string>

using GAArtifactId = std::uint64_t;
inline constexpr GAArtifactId GAInvalidArtifactId = 0;

using GATaskId = std::uint64_t;
inline constexpr GATaskId GAInvalidTaskId = 0;

enum class GAArtifactType : std::uint8_t
{
    Unknown,
    Geometry,
    Mesh,
    Layout,
    SimulationField,
    Image,
    Table,
    Text,
    Binary
};

// Owns metadata only; data is a handle in the associated GADataStore.
struct GAArtifactDescriptor
{
    GAArtifactId id = GAInvalidArtifactId;
    GAArtifactType type = GAArtifactType::Unknown;
    std::uint64_t version = 0;
    std::string name;
    std::string mediaType;
    GADataHandle data = GAInvalidDataHandle;
};

enum class GATaskState : std::uint8_t
{
    Queued,
    Running,
    Completed,
    Cancelled,
    Failed
};

struct GATaskStatus
{
    GATaskId id = GAInvalidTaskId;
    GATaskState state = GATaskState::Queued;
    double progress = 0.0;
    std::string message;
};
