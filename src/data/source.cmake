# -----------------------------------------------------------------------------
# File placement guide: paths are relative to this module. List files for this target, not every file in the directory. See the root source.cmake for general header rules.
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Neutral data API headers in include/: data objects, handles, stores and converters; not backend-specific types.
# -----------------------------------------------------------------------------
set(GA_DATA_CORE_HEADERS
    include/GADataTypes.h
    include/GADataObject.h
    include/GAMeshDataObject.h
    include/GADataStore.h
    include/IGADataConverter.h
    include/GADataConverterRegistry.h
)

# -----------------------------------------------------------------------------
# Implementations in src/ compiled into gaDataCore. Do not add parser, renderer or application code.
# -----------------------------------------------------------------------------
set(GA_DATA_CORE_SOURCES
    src/GADataObject.cpp
    src/GAMeshDataObject.cpp
    src/GADataStore.cpp
    src/IGADataConverter.cpp
    src/GADataConverterRegistry.cpp
)
