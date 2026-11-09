#include "GAOffFileCodec.h"

#include "GACgalMeshFileBackend.h"

GAOffFileCodec::GAOffFileCodec()
    : GAMeshFileCodec(gaCreateCgalMeshFileBackend(GACgalMeshFileFormat::Off))
{
}

const GAFileFormatDescriptor& GAOffFileCodec::descriptor() const noexcept
{
    static constexpr const char* extensions[] = {"off"};
    static constexpr GAFileFormatDescriptor format{"mesh.off",
                                                   "Object File Format",
                                                   extensions,
                                                   1,
                                                   GAFileCapability::Read | GAFileCapability::Write |
                                                       GAFileCapability::Mesh,
                                                   "cgal",
                                                   "CGAL"};
    return format;
}
