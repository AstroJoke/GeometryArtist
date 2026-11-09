# Plugin-internal neutral backend port and file/store bridge. No vendor headers.
set(GA_MESH_FILE_SUPPORT_HEADERS
    include/IGAMeshFileBackend.h
    include/GAMeshFileCodec.h
)
set(GA_MESH_FILE_SUPPORT_SOURCES
    src/GAMeshFileCodec.cpp
)
