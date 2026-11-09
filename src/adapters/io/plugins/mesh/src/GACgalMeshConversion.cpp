#include "GACgalMeshConversion.h"

#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <CGAL/boost/graph/helpers.h>
#include <CGAL/boost/graph/iterator.h>

#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace GACgalMeshConversion
{
using Vertex = Mesh::Vertex_index;
namespace PMP = CGAL::Polygon_mesh_processing;

bool isFinite(const Kernel::Point_3& point)
{
    return std::isfinite(point.x()) && std::isfinite(point.y()) && std::isfinite(point.z());
}

bool hasNondegenerateTriangles(const Mesh& mesh)
{
    if (!CGAL::is_triangle_mesh(mesh))
    {
        return false;
    }
    for (auto face : mesh.faces())
    {
        const auto h = mesh.halfedge(face);
        const auto& a = mesh.point(mesh.source(h));
        const auto& b = mesh.point(mesh.target(h));
        const auto& c = mesh.point(mesh.target(mesh.next(h)));
        if (CGAL::collinear(a, b, c))
        {
            return false;
        }
    }
    return true;
}

std::unique_ptr<GAMeshDataObject> toGA(Mesh& mesh, bool generateNormals)
{
    std::vector<double> positions;
    std::vector<std::uint32_t> indices;
    std::vector<double> normals;
    positions.reserve(mesh.number_of_vertices() * 3);
    indices.reserve(mesh.number_of_faces() * 3);
    // Vertex descriptors may contain gaps after mesh edits. Assign dense GA indices
    // instead of treating CGAL descriptors as offsets into the output arrays.
    auto ids = mesh.add_property_map<Vertex, std::uint32_t>("v:ga_index", 0).first;
    std::uint32_t index = 0;
    for (auto v : mesh.vertices())
    {
        ids[v] = index++;
        const auto& p = mesh.point(v);
        positions.insert(positions.end(), {p.x(), p.y(), p.z()});
    }
    for (auto face : mesh.faces())
    {
        for (auto v : CGAL::vertices_around_face(mesh.halfedge(face), mesh))
        {
            // Keep the face winding from the halfedge cycle; do not sort these indices.
            indices.push_back(ids[v]);
        }
    }
    if (generateNormals)
    {
        auto map = mesh.add_property_map<Vertex, Kernel::Vector_3>("v:ga_normal", CGAL::NULL_VECTOR).first;
        PMP::compute_vertex_normals(mesh, map);
        normals.reserve(positions.size());
        for (auto v : mesh.vertices())
        {
            const auto& n = map[v];
            if (!std::isfinite(n.x()) || !std::isfinite(n.y()) || !std::isfinite(n.z()))
            {
                return nullptr;
            }
            normals.insert(normals.end(), {n.x(), n.y(), n.z()});
        }
    }
    auto result = std::make_unique<GAMeshDataObject>();
    // The view borrows local vectors; setData() copies before they leave scope.
    // Moving result transfers the object pointer, not the local vector allocations.
    const GAMeshDataView view{positions.data(), positions.size() / 3, indices.data(), indices.size(),
                              normals.data(), normals.size() / 3, nullptr, 0};
    return result->setData(view) ? std::move(result) : nullptr;
}

bool fromGA(const GAMeshDataView& view, Mesh& output)
{
    if (view.vertexCount == 0 || view.indexCount == 0 || view.indexCount % 3 != 0 ||
        view.vertexCount > std::numeric_limits<std::uint32_t>::max() ||
        view.vertexCount > std::numeric_limits<std::size_t>::max() / 3 ||
        view.positions == nullptr || view.indices == nullptr)
    {
        return false;
    }
    // Build locally so a rejected face cannot leave the caller's output partially replaced.
    Mesh mesh;
    std::vector<Vertex> vertices;
    vertices.reserve(view.vertexCount);
    for (std::size_t v = 0; v < view.vertexCount; ++v)
    {
        const double* p = view.positions + v * 3;
        for (int axis = 0; axis < 3; ++axis)
        {
            if (!std::isfinite(p[axis]))
            {
                return false;
            }
        }
        vertices.push_back(mesh.add_vertex(Kernel::Point_3(p[0], p[1], p[2])));
    }
    for (std::size_t i = 0; i < view.indexCount; i += 3)
    {
        // Valid indices alone do not guarantee compatible orientation/manifold connectivity.
        // add_face() may reject the topology even when every index is in range.
        const auto a = view.indices[i], b = view.indices[i + 1], c = view.indices[i + 2];
        if (a >= vertices.size() || b >= vertices.size() || c >= vertices.size() ||
            mesh.add_face(vertices[a], vertices[b], vertices[c]) == Mesh::null_face())
        {
            return false;
        }
    }
    if (!hasNondegenerateTriangles(mesh))
    {
        return false;
    }
    output = std::move(mesh);
    return true;
}

} // namespace GACgalMeshConversion
