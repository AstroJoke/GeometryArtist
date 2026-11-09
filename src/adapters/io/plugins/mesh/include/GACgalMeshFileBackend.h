#pragma once

#include "IGAMeshFileBackend.h"

// Private to the CGAL plugin; not a global list of GA-supported formats.
enum class GACgalMeshFileFormat
{
    Obj,
    Off,
    Ply,
    Stl
};

[[nodiscard]] std::unique_ptr<IGAMeshFileBackend> gaCreateCgalMeshFileBackend(GACgalMeshFileFormat format);
