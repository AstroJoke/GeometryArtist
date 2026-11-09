#include "GAStlFileCodec.h"

#include "GACgalMeshFileBackend.h"

GAStlFileCodec::GAStlFileCodec()
    : GAMeshFileCodec(gaCreateCgalMeshFileBackend(GACgalMeshFileFormat::Stl))
{
}

const GAFileFormatDescriptor& GAStlFileCodec::descriptor() const noexcept
{
    static constexpr const char* extensions[] = {"stl"};
    static constexpr GAFileFormatDescriptor format{"mesh.stl",
                                                   "Stereolithography",
                                                   extensions,
                                                   1,
                                                   GAFileCapability::Read | GAFileCapability::Write |
                                                       GAFileCapability::Binary | GAFileCapability::Mesh,
                                                   "cgal",
                                                   "CGAL"};
    return format;
}
