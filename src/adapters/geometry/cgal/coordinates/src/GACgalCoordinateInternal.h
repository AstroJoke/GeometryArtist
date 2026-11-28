#pragma once
#include "GACgalGeometryInternal.h"
#include "GACoordinateFrames.h"

#include <CGAL/Aff_transformation_3.h>
namespace ga::cgal_coordinates
{
using Affine = CGAL::Aff_transformation_3<ga::cgal_geometry::Kernel>;
Affine exactSnapshot(const ga::coordinates::FrameTransformSnapshot& snapshot);
} // namespace ga::cgal_coordinates
