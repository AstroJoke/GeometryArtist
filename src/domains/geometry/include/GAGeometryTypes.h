#pragma once

#include <cstdint>

/**
 * @brief Opaque identifier for an object owned by a geometry-kernel adapter.
 *
 * The value must never be interpreted as a pointer. It is valid only with the
 * IGAGeometryResource instance that created it.
 */
using GAGeometryHandle = std::uint64_t;
inline constexpr GAGeometryHandle GAInvalidGeometryHandle = 0;

enum class GAGeometryRepresentation : std::uint8_t
{
    Unknown,
    PolygonSoup,
    TriangleMesh,
    SurfaceMesh,
    Polyhedron,
    BRep,
    PointCloud
};

enum class GABooleanOperation : std::uint8_t
{
    Difference,
    Union,
    Intersection
};

enum class GAGeometryStatus : std::uint8_t
{
    Success,
    InvalidArgument,
    InvalidGeometry,
    Unsupported,
    Cancelled,
    Failed
};

/**
 * @brief Kernel-independent controls shared by geometry operations.
 *
 * tolerance is a modeling decision, not a universal cure for robustness. Each
 * kernel adapter must document how it interprets zero and positive values.
 */
struct GAGeometryOptions
{
    double tolerance = 0.0;
    bool repairInput = false;
    bool validateOutput = true;
};

/**
 * @brief Result whose output remains owned by the geometry resource.
 */
struct GAGeometryResult
{
    GAGeometryStatus status = GAGeometryStatus::Failed;
    GAGeometryHandle output = GAInvalidGeometryHandle;
    const char* message = nullptr;
};

/// Column/row convention must be defined by the concrete kernel adapter.
struct GATransform
{
    double values[16] = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
};
