#include "GACgalCoordinateAdapter.h"

#include "GACgalCoordinateInternal.h"
#include "GACgalInputPrecision.h"

#include <CGAL/Aff_transformation_3.h>
#include <CGAL/Polygon_mesh_processing/transform.h>
#include <CGAL/boost/graph/convert_nef_polyhedron_to_polygon_mesh.h>
#include <cmath>
#include <stdexcept>

namespace
{
using namespace ga::cgal_geometry;
using Affine = CGAL::Aff_transformation_3<Kernel>;

Affine exactPose(const ga::coordinates::RigidPose& pose, ga::coordinates::PoseInputPrecision precision)
{
    const auto q = precision.quaternionDenominator ? ga::coordinates::normalized(pose.rotation) : pose.rotation;
    const auto& t = pose.translation;
    for (const double value : {q.x, q.y, q.z, q.w, t.x, t.y, t.z})
    {
        if (!std::isfinite(value))
        {
            throw std::invalid_argument("Pose coefficients must be finite");
        }
    }
    using ga::cgal_precision::inputNumber;
    const auto x = inputNumber<Kernel::FT>(q.x, precision.quaternionDenominator);
    const auto y = inputNumber<Kernel::FT>(q.y, precision.quaternionDenominator);
    const auto z = inputNumber<Kernel::FT>(q.z, precision.quaternionDenominator);
    const auto w = inputNumber<Kernel::FT>(q.w, precision.quaternionDenominator);
    const auto normSquared = x * x + y * y + z * z + w * w;
    if (normSquared == 0)
    {
        throw std::invalid_argument("Quaternion must be nonzero");
    }
    const Kernel::FT twice(2);
    // After the explicit input policy, divide by the exact squared norm; R stays rationally orthogonal.
    return {
        (w * w + x * x - y * y - z * z) / normSquared, twice * (x * y - w * z) / normSquared,
        twice * (x * z + w * y) / normSquared,         inputNumber<Kernel::FT>(t.x, precision.translationDenominator),
        twice * (x * y + w * z) / normSquared,         (w * w - x * x + y * y - z * z) / normSquared,
        twice * (y * z - w * x) / normSquared,         inputNumber<Kernel::FT>(t.y, precision.translationDenominator),
        twice * (x * z - w * y) / normSquared,         twice * (y * z + w * x) / normSquared,
        (w * w - x * x - y * y + z * z) / normSquared, inputNumber<Kernel::FT>(t.z, precision.translationDenominator)};
}

} // namespace
namespace ga::cgal_coordinates
{
Affine exactSnapshot(const ga::coordinates::FrameTransformSnapshot& snapshot)
{
    Affine result(CGAL::IDENTITY);
    for (const auto& step : snapshot.steps)
    {
        const auto forward = exactPose(step.localToParent, step.precision);
        // Invert the exact original edge, never an approximately inverted double pose.
        result = (step.inverted ? forward.inverse() : forward) * result;
    }
    return result;
}
} // namespace ga::cgal_coordinates

GACgalCoordinateAdapter::GACgalCoordinateAdapter(GACgalGeometryAdapter& resources) noexcept
    : resources_(resources)
{
}

GAGeometryResult GACgalCoordinateAdapter::createPlaced(GAGeometryHandle source,
                                                       const ga::coordinates::FrameTransformSnapshot& snapshot)
{
    using namespace ga::cgal_geometry;
    try
    {
        // The snapshot is a value and must remain unchanged during this call; no live graph is accessed.
        const auto transform = ga::cgal_coordinates::exactSnapshot(snapshot);
        std::lock_guard lock(resources_.impl_->mutex);
        const auto found = resources_.impl_->resources.find(source);
        if (found == resources_.impl_->resources.end())
        {
            return {GAGeometryStatus::InvalidArgument, GAInvalidGeometryHandle, "Invalid placement source"};
        }
        auto output = std::make_unique<Nef>();
        if (!found->second->is_empty())
        {
            Mesh mesh;
            CGAL::convert_nef_polyhedron_to_polygon_mesh(*found->second, mesh, true);
            CGAL::Polygon_mesh_processing::transform(transform, mesh);
            output = buildSolid(mesh);
            if (!output)
            {
                return {GAGeometryStatus::InvalidGeometry, GAInvalidGeometryHandle, "Invalid placed solid"};
            }
        }
        return {GAGeometryStatus::Success, resources_.impl_->publish(std::move(output)), nullptr};
    }
    catch (const std::invalid_argument&)
    {
        return {GAGeometryStatus::InvalidArgument, GAInvalidGeometryHandle,
                "Expected finite poses and nonzero quaternions"};
    }
    catch (const std::exception&)
    {
        return {GAGeometryStatus::Failed, GAInvalidGeometryHandle, "Exact coordinate placement failed"};
    }
}
