#include "GACgalMeshFileBackend.h"
#include "GACgalMeshConversion.h"

#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/boost/graph/IO/OBJ.h>
#include <CGAL/boost/graph/IO/OFF.h>
#include <CGAL/boost/graph/IO/PLY.h>
#include <CGAL/boost/graph/IO/STL.h>
#include <CGAL/boost/graph/helpers.h>

#include <cmath>
#include <limits>

namespace
{
using GACgalMeshConversion::Mesh;
namespace PMP = CGAL::Polygon_mesh_processing;

bool readMesh(std::istream& input, GACgalMeshFileFormat format, Mesh& mesh)
{
    switch (format)
    {
    case GACgalMeshFileFormat::Obj:
        return CGAL::IO::read_OBJ(input, mesh);
    case GACgalMeshFileFormat::Off:
        return CGAL::IO::read_OFF(input, mesh);
    case GACgalMeshFileFormat::Ply:
        return CGAL::IO::read_PLY(input, mesh);
    case GACgalMeshFileFormat::Stl:
        return CGAL::IO::read_STL(input, mesh);
    default:
        return false;
    }
}

bool writeMesh(std::ostream& output, GACgalMeshFileFormat format, const Mesh& mesh)
{
    const auto options = CGAL::parameters::stream_precision(std::numeric_limits<double>::max_digits10);
    switch (format)
    {
    case GACgalMeshFileFormat::Obj:
        return CGAL::IO::write_OBJ(output, mesh, options);
    case GACgalMeshFileFormat::Off:
        return CGAL::IO::write_OFF(output, mesh, options);
    case GACgalMeshFileFormat::Ply:
        return CGAL::IO::write_PLY(output, mesh, options);
    case GACgalMeshFileFormat::Stl:
        return CGAL::IO::write_STL(output, mesh, options);
    default:
        return false;
    }
}

class GACgalMeshFileBackend : public IGAMeshFileBackend
{
public:
    explicit GACgalMeshFileBackend(GACgalMeshFileFormat format)
        : format_(format)
    {
    }

    GAMeshDecodeResult decode(std::istream& input, const GAMeshDecodeOptions& options) override
    {
        Mesh mesh;
        if (!readMesh(input, format_, mesh) || mesh.number_of_faces() == 0 || !CGAL::is_valid_polygon_mesh(mesh))
        {
            return {GAFileStatus::ParseFailed, nullptr, "CGAL requires a valid, nonempty manifold polygon mesh"};
        }
        for (auto v : mesh.vertices())
        {
            // Reject NaN/infinity before geometric predicates or triangulation use them.
            if (!GACgalMeshConversion::isFinite(mesh.point(v)))
            {
                return {GAFileStatus::ParseFailed, nullptr, "Non-finite vertex coordinate"};
            }
        }
        if (!CGAL::is_triangle_mesh(mesh))
        {
            // Triangulation is an explicit import choice, not an implicit conversion side effect.
            if (!options.triangulate)
            {
                return {GAFileStatus::Unsupported, nullptr, "GA mesh data requires triangles; enable triangulation"};
            }
            if (!PMP::triangulate_faces(mesh) || !CGAL::is_triangle_mesh(mesh))
            {
                return {GAFileStatus::ParseFailed, nullptr, "Polygon triangulation failed"};
            }
        }
        const std::size_t vertices = mesh.number_of_vertices();
        const std::size_t faces = mesh.number_of_faces();
        if (!GACgalMeshConversion::hasNondegenerateTriangles(mesh) ||
            vertices > std::numeric_limits<std::uint32_t>::max() ||
            vertices > std::numeric_limits<std::size_t>::max() / 3 ||
            faces > std::numeric_limits<std::size_t>::max() / 3)
        {
            return {GAFileStatus::ParseFailed, nullptr, "Degenerate triangles or mesh exceeds GA buffer limits"};
        }
        auto object = GACgalMeshConversion::toGA(mesh, options.generateNormals);
        if (!object)
        {
            return {GAFileStatus::ParseFailed, nullptr, "Cannot convert CGAL mesh to GA buffers"};
        }
        return {GAFileStatus::Success, std::move(object),
                "Imported geometry only; source attributes are not preserved by this CGAL adapter"};
    }

    GAMeshEncodeResult encode(std::ostream& output, const GAMeshDataView& view, bool binary) override
    {
        Mesh mesh;
        if (!GACgalMeshConversion::fromGA(view, mesh))
        {
            return {GAFileStatus::InvalidArgument, "CGAL cannot encode this topology or coordinate range"};
        }
        // Binary STL stores float coordinates; this restriction belongs to the file adapter.
        if (binary && format_ == GACgalMeshFileFormat::Stl)
        {
            for (std::size_t i = 0; i < view.vertexCount * 3; ++i)
            {
                if (std::abs(view.positions[i]) > std::numeric_limits<float>::max())
                {
                    return {GAFileStatus::InvalidArgument, "Coordinate exceeds binary STL range"};
                }
            }
            // Validate the coordinates that binary STL will actually store.
            for (auto vertex : mesh.vertices())
            {
                const auto point = mesh.point(vertex);
                mesh.point(vertex) = GACgalMeshConversion::Kernel::Point_3(
                    static_cast<float>(point.x()), static_cast<float>(point.y()), static_cast<float>(point.z()));
            }
            if (!GACgalMeshConversion::hasNondegenerateTriangles(mesh))
            {
                return {GAFileStatus::InvalidArgument, "Binary STL precision would collapse a triangle"};
            }
        }
        CGAL::IO::set_mode(output, binary ? CGAL::IO::BINARY : CGAL::IO::ASCII);
        if (!writeMesh(output, format_, mesh))
        {
            return {GAFileStatus::WriteFailed, "CGAL mesh serialization failed"};
        }
        return {GAFileStatus::Success, "Exported geometry only; vertex attributes are not serialized"};
    }

private:
    GACgalMeshFileFormat format_;
};
} // namespace

std::unique_ptr<IGAMeshFileBackend> gaCreateCgalMeshFileBackend(GACgalMeshFileFormat format)
{
    return std::make_unique<GACgalMeshFileBackend>(format);
}
