#include "GACgalGeometryInternal.h"

#include <CGAL/Aff_transformation_3.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/transform.h>
#include <CGAL/boost/graph/convert_nef_polyhedron_to_polygon_mesh.h>
#include <CGAL/convex_hull_3.h>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace
{
using namespace ga::cgal_geometry;
using Affine = CGAL::Aff_transformation_3<Kernel>;
namespace PMP = CGAL::Polygon_mesh_processing;
constexpr std::size_t MatrixWidth = 4;
constexpr std::size_t AffineRowCount = 3;
constexpr std::size_t AffineCoefficientCount = MatrixWidth * AffineRowCount;
constexpr std::size_t MinimumSolidPoints = 4;

Affine exactTransform(const GATransform& transform)
{
    for (const double value : transform.values)
    {
        if (!std::isfinite(value))
        {
            throw std::invalid_argument("Transform contains a non-finite coefficient");
        }
    }
    const auto* m = transform.values;
    constexpr std::size_t LastRow = AffineCoefficientCount;
    if (m[LastRow] != 0 || m[LastRow + 1] != 0 || m[LastRow + 2] != 0 || m[LastRow + 3] != 1)
    {
        throw std::invalid_argument("Expected an affine matrix with last row [0,0,0,1]");
    }
    std::array<Kernel::FT, AffineCoefficientCount> a;
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        // Preserve exact binary double coefficients, never re-quantize computed coordinates.
        a[i] = Kernel::FT(m[i]);
    }
    const auto determinant =
        a[0] * (a[5] * a[10] - a[6] * a[9]) - a[1] * (a[4] * a[10] - a[6] * a[8]) + a[2] * (a[4] * a[9] - a[5] * a[8]);
    if (determinant == 0)
    {
        throw std::invalid_argument("Singular transform cannot preserve a solid");
    }
    return {a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11]};
}

std::unique_ptr<Nef> transformedSolid(const Nef& source, const GATransform& transform)
{
    const auto affine = exactTransform(transform);
    if (source.is_empty())
    {
        return std::make_unique<Nef>();
    }
    // Transform an exact mesh and rebuild, avoiding repeated Nef::transform or double snapshots.
    Mesh mesh;
    CGAL::convert_nef_polyhedron_to_polygon_mesh(source, mesh, true);
    PMP::transform(affine, mesh);
    if (affine.is_odd())
    {
        // Reflection reverses orientation; reverse all shells to preserve cavity relationships.
        PMP::reverse_face_orientations(mesh);
    }
    auto result = buildSolid(mesh);
    if (!result)
    {
        throw std::runtime_error("Transformed boundary does not represent a valid bounded solid");
    }
    return result;
}
} // namespace

GAGeometryResult GACgalGeometryAdapter::createTransformed(GAGeometryHandle geometry, const GATransform& transform)
{
    try
    {
        std::lock_guard lock(impl_->mutex);
        const auto found = impl_->resources.find(geometry);
        if (found == impl_->resources.end())
        {
            return {GAGeometryStatus::InvalidArgument, GAInvalidGeometryHandle, "Invalid or released geometry handle"};
        }
        auto result = transformedSolid(*found->second, transform);
        return {GAGeometryStatus::Success, impl_->publish(std::move(result)), nullptr};
    }
    catch (const std::invalid_argument&)
    {
        return {GAGeometryStatus::InvalidArgument, GAInvalidGeometryHandle,
                "Expected a finite nonsingular affine matrix"};
    }
    catch (const std::exception&)
    {
        return {GAGeometryStatus::Failed, GAInvalidGeometryHandle, "CGAL exact transform failed"};
    }
}

GAGeometryStatus GACgalGeometryAdapter::transform(GAGeometryHandle geometry, const GATransform& transform)
{
    try
    {
        std::lock_guard lock(impl_->mutex);
        const auto found = impl_->resources.find(geometry);
        if (found == impl_->resources.end())
        {
            return GAGeometryStatus::InvalidArgument;
        }
        auto result = transformedSolid(*found->second, transform);
        // Keep the legacy in-place API, committing the replacement only after full success.
        found->second = std::move(result);
        return GAGeometryStatus::Success;
    }
    catch (const std::invalid_argument&)
    {
        return GAGeometryStatus::InvalidArgument;
    }
    catch (const std::exception&)
    {
        return GAGeometryStatus::Failed;
    }
}

GAGeometryResult GACgalGeometryAdapter::computeHull(const GAGeometryHandle* geometries, std::size_t count,
                                                    const GAGeometryOptions& options)
{
    if (geometries == nullptr || count == 0 || !std::isfinite(options.tolerance) || options.tolerance < 0)
    {
        return {GAGeometryStatus::InvalidArgument, GAInvalidGeometryHandle, "Invalid hull inputs or tolerance"};
    }
    if (options.tolerance != 0 || options.repairInput)
    {
        return {GAGeometryStatus::Unsupported, GAInvalidGeometryHandle,
                "Exact hull construction does not apply tolerance or implicit repair"};
    }
    try
    {
        std::lock_guard lock(impl_->mutex);
        std::vector<Kernel::Point_3> points;
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto found = impl_->resources.find(geometries[i]);
            if (found == impl_->resources.end())
            {
                return {GAGeometryStatus::InvalidArgument, GAInvalidGeometryHandle, "Invalid hull geometry handle"};
            }
            // Collect exact Nef vertices directly without exporting/copying input meshes.
            for (auto vertex = found->second->vertices_begin(); vertex != found->second->vertices_end(); ++vertex)
            {
                points.push_back(vertex->point());
            }
        }
        auto result = std::make_unique<Nef>();
        if (!points.empty())
        {
            if (points.size() < MinimumSolidPoints)
            {
                return {GAGeometryStatus::InvalidGeometry, GAInvalidGeometryHandle, "Hull does not enclose a solid"};
            }
            Mesh hull;
            CGAL::convex_hull_3(points.begin(), points.end(), hull);
            result = buildSolid(hull, options.validateOutput);
            if (!result || result->is_empty())
            {
                return {GAGeometryStatus::InvalidGeometry, GAInvalidGeometryHandle,
                        "Generated hull is not a valid solid"};
            }
        }
        // No implicit remesh/retry on failure; subsequent policy belongs to the domain.
        return {GAGeometryStatus::Success, impl_->publish(std::move(result)), nullptr};
    }
    catch (const std::exception&)
    {
        return {GAGeometryStatus::Failed, GAInvalidGeometryHandle, "CGAL convex hull construction failed"};
    }
}
