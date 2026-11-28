#pragma once

#include "GAMeshDataObject.h"

#include <cmath>
#include <memory>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace ga::cgal_test
{
constexpr std::size_t CoordinateCount = 3;
constexpr std::size_t TriangleCornerCount = 3;
constexpr std::uint32_t CylinderSegments = 16;
constexpr double CylinderRadius = 0.5;
constexpr double CylinderMinZ = -0.5;
constexpr double CylinderMaxZ = 2.5;
constexpr double VolumeTolerance = 1e-9;

inline void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

inline std::unique_ptr<GAMeshDataObject> mesh(const std::vector<double>& points,
                                              const std::vector<std::uint32_t>& triangles)
{
    auto result = std::make_unique<GAMeshDataObject>();
    require(result->setData({points.data(), points.size() / CoordinateCount, triangles.data(), triangles.size()}),
            "Fixture data rejected");
    return result;
}

inline std::unique_ptr<GAMeshDataObject> cube(double low = 0, double high = 2)
{
    return mesh(
        {low, low, low,  high, low, low,  high, high, low,  low, high, low,
         low, low, high, high, low, high, high, high, high, low, high, high},
        {0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4, 1, 2, 6, 1, 6, 5, 2, 3, 7, 2, 7, 6, 3, 0, 4, 3, 4, 7});
}

inline std::unique_ptr<GAMeshDataObject> cylinder()
{
    std::vector<double> points;
    std::vector<std::uint32_t> triangles;
    for (double z : {CylinderMinZ, CylinderMaxZ})
    {
        for (std::uint32_t i = 0; i < CylinderSegments; ++i)
        {
            const double angle = 2 * std::numbers::pi * i / CylinderSegments;
            points.insert(points.end(),
                          {1 + CylinderRadius * std::cos(angle), 1 + CylinderRadius * std::sin(angle), z});
        }
    }
    constexpr auto BottomCenter = 2 * CylinderSegments;
    constexpr auto TopCenter = BottomCenter + 1;
    points.insert(points.end(), {1, 1, CylinderMinZ, 1, 1, CylinderMaxZ});
    for (std::uint32_t i = 0; i < CylinderSegments; ++i)
    {
        const auto j = (i + 1) % CylinderSegments;
        triangles.insert(triangles.end(), {BottomCenter, j, i, TopCenter, i + CylinderSegments, j + CylinderSegments, i,
                                           j, j + CylinderSegments, i, j + CylinderSegments, i + CylinderSegments});
    }
    return mesh(points, triangles);
}

inline double signedVolume(const GAMeshDataObject& object)
{
    const auto view = object.view();
    double sum = 0;
    for (std::size_t i = 0; i < view.indexCount; i += TriangleCornerCount)
    {
        const auto* a = view.positions + CoordinateCount * view.indices[i];
        const auto* b = view.positions + CoordinateCount * view.indices[i + 1];
        const auto* c = view.positions + CoordinateCount * view.indices[i + 2];
        sum += a[0] * (b[1] * c[2] - b[2] * c[1]) + a[1] * (b[2] * c[0] - b[0] * c[2]) +
               a[2] * (b[0] * c[1] - b[1] * c[0]);
    }
    constexpr double TetrahedronVolumeDivisor = 6;
    return sum / TetrahedronVolumeDivisor;
}

inline double cylinderArea()
{
    return CylinderSegments * CylinderRadius * CylinderRadius * std::sin(2 * std::numbers::pi / CylinderSegments) / 2;
}
} // namespace ga::cgal_test
