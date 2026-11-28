#include "GACgalGeometryInternal.h"
#include "GACgalInputPrecision.h"
#include "GADataStore.h"
#include "GAMeshDataObject.h"

#include <CGAL/Polygon_mesh_processing/connected_components.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/self_intersections.h>
#include <CGAL/boost/graph/convert_nef_polyhedron_to_polygon_mesh.h>
#include <CGAL/boost/graph/helpers.h>
#include <CGAL/boost/graph/iterator.h>
#include <atomic>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace ga::cgal_geometry
{
namespace PMP = CGAL::Polygon_mesh_processing;

constexpr std::size_t CoordinateCount = 3;
constexpr std::size_t TriangleCornerCount = 3;

GAGeometryHandle allocateHandle()
{
    // Only identity allocation is shared, never geometry. Exhaustion fails without reuse.
    static std::atomic<GAGeometryHandle> next{1};
    auto value = next.load(std::memory_order_relaxed);
    while (value != std::numeric_limits<GAGeometryHandle>::max())
    {
        if (next.compare_exchange_weak(value, value + 1, std::memory_order_relaxed))
        {
            return value;
        }
    }
    throw std::overflow_error("Geometry handle space exhausted");
}

Kernel::FT importCoordinate(double value, const GAGeometryInputPrecision& policy)
{
    return ga::cgal_precision::inputNumber<Kernel::FT>(value, policy.coordinateDenominator);
}

std::unique_ptr<Nef> importMesh(const GAMeshDataView& view, const GAGeometryInputPrecision& policy)
{
    if (view.vertexCount == 0 && view.indexCount == 0)
    {
        return std::make_unique<Nef>();
    }
    if (view.vertexCount == 0 || view.indexCount == 0 || view.indexCount % TriangleCornerCount != 0 ||
        view.positions == nullptr || view.indices == nullptr ||
        view.vertexCount > std::numeric_limits<std::uint32_t>::max())
    {
        return nullptr;
    }
    Mesh mesh;
    std::vector<Mesh::Vertex_index> vertices;
    vertices.reserve(view.vertexCount);
    for (std::size_t i = 0; i < view.vertexCount; ++i)
    {
        const double* point = view.positions + i * CoordinateCount;
        vertices.push_back(mesh.add_vertex({importCoordinate(point[0], policy), importCoordinate(point[1], policy),
                                            importCoordinate(point[2], policy)}));
    }
    for (std::size_t i = 0; i < view.indexCount; i += TriangleCornerCount)
    {
        const auto a = view.indices[i], b = view.indices[i + 1], c = view.indices[i + 2];
        if (a >= vertices.size() || b >= vertices.size() || c >= vertices.size() ||
            CGAL::collinear(mesh.point(vertices[a]), mesh.point(vertices[b]), mesh.point(vertices[c])) ||
            mesh.add_face(vertices[a], vertices[b], vertices[c]) == Mesh::null_face())
        {
            return nullptr;
        }
    }
    for (auto vertex : mesh.vertices())
    {
        if (mesh.is_isolated(vertex))
        {
            return nullptr;
        }
    }
    return buildSolid(mesh);
}

std::unique_ptr<Nef> buildSolid(const Mesh& mesh, bool validateOutput)
{
    if (mesh.is_empty())
    {
        return std::make_unique<Nef>();
    }
    if (!CGAL::is_valid_polygon_mesh(mesh) || !CGAL::is_triangle_mesh(mesh) || !CGAL::is_closed(mesh))
    {
        return nullptr;
    }
    for (const auto face : mesh.faces())
    {
        const auto edge = mesh.halfedge(face);
        if (CGAL::collinear(mesh.point(mesh.source(edge)), mesh.point(mesh.target(edge)),
                            mesh.point(mesh.target(mesh.next(edge)))))
        {
            return nullptr;
        }
    }
    // Check self-intersections before evaluating the volume predicates.
    if (PMP::does_self_intersect(mesh) || !PMP::does_bound_a_volume(mesh) || PMP::volume(mesh) <= 0)
    {
        return nullptr;
    }
    // Nef(mesh) marks every bounded volume, filling cavities in a multi-shell mesh.
    // After validating the boundary, combine closed shells by parity to retain nested cavities/islands.
    std::vector<Mesh> shells;
    PMP::split_connected_components(mesh, shells);
    auto result = std::make_unique<Nef>();
    for (const auto& shell : shells)
    {
        *result = result->symmetric_difference(Nef(shell));
    }
    *result = result->regularization();
    if ((validateOutput && !result->is_valid()) || !result->is_simple() || result->volumes_begin()->mark())
    {
        return nullptr;
    }
    return result;
}

std::unique_ptr<GAMeshDataObject> exportMesh(const Nef& nef, const char* name)
{
    auto result = std::make_unique<GAMeshDataObject>(name != nullptr ? name : "Geometry");
    if (nef.is_empty())
    {
        return result;
    }
    Mesh mesh;
    CGAL::convert_nef_polyhedron_to_polygon_mesh(nef, mesh, true);
    if (!CGAL::is_triangle_mesh(mesh) || mesh.number_of_vertices() > std::numeric_limits<std::uint32_t>::max())
    {
        return nullptr;
    }
    std::vector<double> positions;
    std::vector<std::uint32_t> indices;
    positions.reserve(mesh.number_of_vertices() * CoordinateCount);
    indices.reserve(mesh.number_of_faces() * TriangleCornerCount);
    auto ids = mesh.add_property_map<Mesh::Vertex_index, std::uint32_t>("v:ga_export_index", 0).first;
    std::uint32_t next = 0;
    for (auto vertex : mesh.vertices())
    {
        ids[vertex] = next++;
        const auto& point = mesh.point(vertex);
        const double xyz[] = {CGAL::to_double(point.x()), CGAL::to_double(point.y()), CGAL::to_double(point.z())};
        for (const double coordinate : xyz)
        {
            if (!std::isfinite(coordinate))
            {
                return nullptr;
            }
            positions.push_back(coordinate);
        }
    }
    for (auto face : mesh.faces())
    {
        for (auto vertex : CGAL::vertices_around_face(mesh.halfedge(face), mesh))
        {
            indices.push_back(ids[vertex]);
        }
    }
    const GAMeshDataView view{positions.data(), mesh.number_of_vertices(), indices.data(), indices.size()};
    return result->setData(view) ? std::move(result) : nullptr;
}
} // namespace ga::cgal_geometry

using namespace ga::cgal_geometry;

GAGeometryHandle GACgalGeometryAdapter::Impl::publish(std::unique_ptr<Nef> geometry)
{
    const auto handle = allocateHandle();
    resources.emplace(handle, std::move(geometry));
    return handle;
}

GACgalGeometryAdapter::GACgalGeometryAdapter(GAGeometryInputPrecision policy)
    : impl_(std::make_unique<Impl>(policy))
{
}
GACgalGeometryAdapter::~GACgalGeometryAdapter() = default;

GAGeometryHandle GACgalGeometryAdapter::importGeometry(const GADataObject& source)
{
    const auto* mesh = dynamic_cast<const GAMeshDataObject*>(&source);
    if (mesh == nullptr)
    {
        return GAInvalidGeometryHandle;
    }
    try
    {
        std::lock_guard lock(impl_->mutex);
        auto geometry = importMesh(mesh->view(), impl_->policy);
        return geometry ? impl_->publish(std::move(geometry)) : GAInvalidGeometryHandle;
    }
    catch (const std::exception&)
    {
        return GAInvalidGeometryHandle;
    }
}

GADataHandle GACgalGeometryAdapter::exportGeometry(GAGeometryHandle geometry, const char* name,
                                                   GADataStore& destination) const
{
    try
    {
        std::unique_ptr<GAMeshDataObject> snapshot;
        {
            std::lock_guard lock(impl_->mutex);
            const auto found = impl_->resources.find(geometry);
            if (found == impl_->resources.end())
            {
                return GAInvalidDataHandle;
            }
            // Double conversion is a display/interchange snapshot, never an exact-state update.
            snapshot = exportMesh(*found->second, name);
        }
        return snapshot ? destination.registe(std::move(snapshot)) : GAInvalidDataHandle;
    }
    catch (const std::exception&)
    {
        return GAInvalidDataHandle;
    }
}

void GACgalGeometryAdapter::releaseGeometry(GAGeometryHandle geometry) noexcept
{
    std::lock_guard lock(impl_->mutex);
    impl_->resources.erase(geometry);
}

bool GACgalGeometryAdapter::isValid(GAGeometryHandle geometry) const noexcept
{
    std::lock_guard lock(impl_->mutex);
    return impl_->resources.contains(geometry);
}

GAGeometryRepresentation GACgalGeometryAdapter::representation(GAGeometryHandle geometry) const noexcept
{
    return isValid(geometry) ? GAGeometryRepresentation::BRep : GAGeometryRepresentation::Unknown;
}

std::size_t GACgalGeometryAdapter::resourceCount() const noexcept
{
    std::lock_guard lock(impl_->mutex);
    return impl_->resources.size();
}

GAGeometryResult GACgalGeometryAdapter::apply(GABooleanOperation operation, GAGeometryHandle left,
                                              GAGeometryHandle right, const GAGeometryOptions& options)
{
    if (!std::isfinite(options.tolerance) || options.tolerance < 0)
    {
        return {GAGeometryStatus::InvalidArgument, GAInvalidGeometryHandle, "Invalid tolerance"};
    }
    if (options.tolerance != 0 || options.repairInput)
    {
        return {GAGeometryStatus::Unsupported, GAInvalidGeometryHandle,
                "Exact Boolean operations do not apply tolerance or implicit repair"};
    }
    try
    {
        std::lock_guard lock(impl_->mutex);
        const auto a = impl_->resources.find(left), b = impl_->resources.find(right);
        if (a == impl_->resources.end() || b == impl_->resources.end())
        {
            return {GAGeometryStatus::InvalidArgument, GAInvalidGeometryHandle, "Invalid or released geometry handle"};
        }
        auto result = std::make_unique<Nef>();
        switch (operation)
        {
        case GABooleanOperation::Difference:
            *result = *a->second - *b->second;
            break;
        case GABooleanOperation::Union:
            *result = *a->second + *b->second;
            break;
        case GABooleanOperation::Intersection:
            *result = *a->second * *b->second;
            break;
        default:
            return {GAGeometryStatus::Unsupported, GAInvalidGeometryHandle, "Unsupported Boolean operation"};
        }
        // Regularized solid semantics discard isolated lower-dimensional sets.
        *result = result->regularization();
        if (!result->is_simple() || (options.validateOutput && !result->is_valid()))
        {
            return {GAGeometryStatus::InvalidGeometry, GAInvalidGeometryHandle,
                    "Result cannot be represented as a manifold solid boundary"};
        }
        // Publish only after success; failures never replace inputs or expose partial results.
        return {GAGeometryStatus::Success, impl_->publish(std::move(result)), nullptr};
    }
    catch (const std::exception&)
    {
        // Return a static message, never a dangling exception.what() pointer.
        return {GAGeometryStatus::Failed, GAInvalidGeometryHandle, "CGAL Boolean operation failed"};
    }
}
