# Only this backend's implementation belongs here; common orchestration lives in ../common.
set(GA_CGAL_FILE_HEADERS
    include/GACgalMeshFileBackend.h
    include/GACgalMeshConversion.h
    include/GAObjFileCodec.h
    include/GAOffFileCodec.h
    include/GAPlyFileCodec.h
    include/GAStlFileCodec.h
)
set(GA_CGAL_FILE_SOURCES
    src/GACgalMeshFileBackend.cpp
    src/GACgalMeshConversion.cpp
    src/GAObjFileCodec.cpp
    src/GAOffFileCodec.cpp
    src/GAPlyFileCodec.cpp
    src/GAStlFileCodec.cpp
    src/GAMeshFilePlugin.cpp
)
