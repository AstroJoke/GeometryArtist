#pragma once

#include "GACgalGeometryAdapter.h"

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/Surface_mesh.h>
#include <memory>
#include <mutex>
#include <unordered_map>

// Shared implementation details, never a public header or neutral contract.
namespace ga::cgal_geometry
{
using Kernel = CGAL::Exact_predicates_exact_constructions_kernel;
using Mesh = CGAL::Surface_mesh<Kernel::Point_3>;
using Nef = CGAL::Nef_polyhedron_3<Kernel>;

std::unique_ptr<Nef> buildSolid(const Mesh& mesh, bool validateOutput = true);
} // namespace ga::cgal_geometry

class GACgalGeometryAdapter::Impl
{
public:
    explicit Impl(GAGeometryInputPrecision value)
        : policy(value)
    {
    }

    GAGeometryHandle publish(std::unique_ptr<ga::cgal_geometry::Nef> geometry);

    GAGeometryInputPrecision policy;
    // Serialize even const/lazy evaluation per instance; independent copies may run in separate instances.
    mutable std::mutex mutex;
    std::unordered_map<GAGeometryHandle, std::unique_ptr<ga::cgal_geometry::Nef>> resources;
};
