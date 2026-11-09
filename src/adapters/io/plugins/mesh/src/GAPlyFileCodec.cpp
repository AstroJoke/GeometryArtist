#include "GAPlyFileCodec.h"

#include "GACgalMeshFileBackend.h"

GAPlyFileCodec::GAPlyFileCodec()
    : GAMeshFileCodec(gaCreateCgalMeshFileBackend(GACgalMeshFileFormat::Ply))
{
}

const GAFileFormatDescriptor& GAPlyFileCodec::descriptor() const noexcept
{
    static constexpr const char* extensions[] = {"ply"};
    static constexpr GAFileFormatDescriptor format{"mesh.ply",
                                                   "Polygon File Format",
                                                   extensions,
                                                   1,
                                                   GAFileCapability::Read | GAFileCapability::Write |
                                                       GAFileCapability::Binary | GAFileCapability::Mesh,
                                                   "cgal",
                                                   "CGAL"};
    return format;
}
