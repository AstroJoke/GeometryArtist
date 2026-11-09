#include "GAObjFileCodec.h"

#include "GACgalMeshFileBackend.h"

GAObjFileCodec::GAObjFileCodec()
    : GAMeshFileCodec(gaCreateCgalMeshFileBackend(GACgalMeshFileFormat::Obj))
{
}

const GAFileFormatDescriptor& GAObjFileCodec::descriptor() const noexcept
{
    static constexpr const char* extensions[] = {"obj"};
    static constexpr GAFileFormatDescriptor format{
        "mesh.obj",
        "Wavefront OBJ",
        extensions,
        1,
        GAFileCapability::Read | GAFileCapability::Write | GAFileCapability::Mesh,
        "cgal",
        "CGAL"};
    return format;
}
