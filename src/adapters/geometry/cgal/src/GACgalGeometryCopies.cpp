#include "GACgalGeometryInternal.h"

#include <CGAL/boost/graph/convert_nef_polyhedron_to_polygon_mesh.h>
#include <CGAL/number_utils.h>

GAGeometryResult GACgalGeometryAdapter::copyGeometryFrom(const GACgalGeometryAdapter& source, GAGeometryHandle geometry)
{
    using namespace ga::cgal_geometry;
    try
    {
        Mesh detached;
        {
            std::lock_guard sourceLock(source.impl_->mutex);
            const auto found = source.impl_->resources.find(geometry);
            if (found == source.impl_->resources.end())
            {
                return {GAGeometryStatus::InvalidArgument, GAInvalidGeometryHandle, "Invalid copy source"};
            }
            if (!found->second->is_empty())
            {
                CGAL::convert_nef_polyhedron_to_polygon_mesh(*found->second, detached, true);
                for (const auto vertex : detached.vertices())
                {
                    const auto point = detached.point(vertex);
                    // Materialize exact values under the source lock into fresh lazy leaves.
                    // No shared Nef/DAG, quantization or reduction of rational bit lengths.
                    detached.point(vertex) = {Kernel::FT(CGAL::exact(point.x())), Kernel::FT(CGAL::exact(point.y())),
                                              Kernel::FT(CGAL::exact(point.z()))};
                }
            }
        }
        // Never hold both resource locks; self-copy and opposite-direction copies cannot form a lock cycle.
        std::lock_guard destinationLock(impl_->mutex);
        auto output = buildSolid(detached);
        if (!output)
        {
            return {GAGeometryStatus::InvalidGeometry, GAInvalidGeometryHandle, "Invalid detached solid"};
        }
        return {GAGeometryStatus::Success, impl_->publish(std::move(output)), nullptr};
    }
    catch (const std::exception&)
    {
        return {GAGeometryStatus::Failed, GAInvalidGeometryHandle, "Exact resource copy failed"};
    }
}
