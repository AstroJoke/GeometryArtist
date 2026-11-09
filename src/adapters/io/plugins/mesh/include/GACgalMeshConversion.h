#pragma once

#include "GAMeshDataObject.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>

#include <memory>

/**
 * @brief Private EPICK mesh conversion, independent of file formats and streams.
 *
 * No runtime/public GA contract exposes these CGAL types. Keep a concrete kernel
 * until a second real consumer requires another one; templates are not needed yet.
 * GA buffers contain doubles, not a persistence format for exact coordinates.
 *
 */
namespace GACgalMeshConversion
{
using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using Mesh = CGAL::Surface_mesh<Kernel::Point_3>;

[[nodiscard]] bool isFinite(const Kernel::Point_3& point);
/**
 * @pre Connectivity is valid and coordinates are finite.
 * @return False for non-triangle faces or collinear triangle vertices.
 * Does not check self-intersections, closedness, or whether the mesh is nonempty.
 *
 */
[[nodiscard]] bool hasNondegenerateTriangles(const Mesh& mesh);

// Input must already be a valid triangle mesh with finite coordinates and GA-sized buffers.
// Adds/reuses indexing/normal maps on the input; does not remove them afterwards.
// Copies local buffers into an independent GA object; this is not zero-copy.
// Returns nullptr on normal/assignment failure; allocation exceptions propagate.
[[nodiscard]] std::unique_ptr<GAMeshDataObject> toGA(Mesh& mesh, bool generateNormals);

// Converts geometry only; validates finite coordinates, indices and CGAL topology.
// Leaves output unchanged if validation fails. No format-specific range restrictions.
// Borrows the input for this call only; source normals and UVs are not transferred.
[[nodiscard]] bool fromGA(const GAMeshDataView& view, Mesh& output);
} // namespace GACgalMeshConversion
