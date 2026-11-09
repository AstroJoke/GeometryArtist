#include "GAFilePlugin.h"
#include "GAObjFileCodec.h"
#include "GAOffFileCodec.h"
#include "GAPlyFileCodec.h"
#include "GAStlFileCodec.h"

#if defined(_WIN32)
#define GA_MESH_PLUGIN_EXPORT __declspec(dllexport)
#else
#define GA_MESH_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

namespace
{
constexpr GAFilePluginDescriptor descriptor{GAFilePluginAbiVersion, "geometryartist.file.mesh",
                                            "GeometryArtist CGAL Mesh File Codecs", 2, 4};
}

extern "C" GA_MESH_PLUGIN_EXPORT const GAFilePluginDescriptor* gaQueryFilePlugin() noexcept
{
    return &descriptor;
}

extern "C" GA_MESH_PLUGIN_EXPORT GAFileCodec* gaCreateFileCodec(std::size_t index)
{
    switch (index)
    {
    case 0:
        return new GAObjFileCodec();
    case 1:
        return new GAOffFileCodec();
    case 2:
        return new GAPlyFileCodec();
    case 3:
        return new GAStlFileCodec();
    default:
        return nullptr;
    }
}

extern "C" GA_MESH_PLUGIN_EXPORT void gaDestroyFileCodec(GAFileCodec* codec) noexcept
{
    delete codec;
}
