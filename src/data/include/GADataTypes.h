#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @brief Stable identifier used to refer to data owned by GADataStore.
 *
 * A handle is neither a pointer nor an ownership token. Its validity must be
 * checked against the store that created it.
 *
 */
using GADataHandle = std::uint64_t;
using GADataObjectId = std::uint64_t;
using GADataRevision = std::uint64_t;

inline constexpr GADataHandle GAInvalidDataHandle = 0;
inline constexpr GADataObjectId GAInvalidDataObjectId = 0;

enum class GADataObjectType : std::uint16_t
{
    Unknown,
    TriangleMesh,
    PolygonMesh,
    PointCloud,
    Polyline,
    Image,
    Volume,
    Scene,
    Layout,
    SimulationField,
    Table,
    AIEmbedding
};

enum class GADataStatus : std::uint8_t
{
    Success,
    InvalidArgument,
    NotFound,
    TypeMismatch,
    Unsupported,
    Cancelled,
    Failed
};

/**
 * @brief Non-owning, contiguous view of triangle-mesh buffers.
 *
 * Positions and normals contain three scalar values per element; texture
 * coordinates contain two. The pointed-to memory must remain valid only for the
 * duration of the receiving call unless that API explicitly states otherwise.
 *
 */
struct GAMeshDataView
{
    const double* positions = nullptr;
    std::size_t vertexCount = 0;
    const std::uint32_t* indices = nullptr;
    std::size_t indexCount = 0;
    const double* normals = nullptr;
    std::size_t normalCount = 0;
    const double* textureCoordinates = nullptr;
    std::size_t textureCoordinateCount = 0;
};

/// Options shared by neutral-data converters.
struct GAConversionOptions
{
    double tolerance = 0.0;
    bool triangulate = true;
    bool generateNormals = false;
};

/// Result of a conversion, including the newly registered output handle.
struct GAConversionResult
{
    GADataStatus status = GADataStatus::Failed;
    GADataHandle output = GAInvalidDataHandle;
    const char* message = nullptr;
};
