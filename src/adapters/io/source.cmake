# -----------------------------------------------------------------------------
# File placement guide: paths are relative to this module. List files for this target, not every file in the directory. See the root source.cmake for general header rules.
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# FileIO library headers in include/: services, registry, loader and policies. Backend file lists live in their own plugin directories.
# -----------------------------------------------------------------------------
set(GA_FILE_IO_HEADERS
    include/GAFileTypes.h
    include/IGAFileReader.h
    include/IGAFileWriter.h
    include/GAFileCodec.h
    include/GAFileCodecRegistry.h
    include/GAFilePropertyPolicy.h
    include/GAFileCodecPolicy.h
    include/GAFileObservationPolicy.h
    include/GAFileExecutionPolicy.h
    include/GAFileIOAssembly.h
    include/GAFileIOService.h
    include/GAFilePlugin.h
    include/GAFilePluginLoader.h
)

# -----------------------------------------------------------------------------
# Minimal reader/writer contracts forwarded to consumers. A new contract can also appear in GA_FILE_IO_HEADERS; that overlap is intentional.
# -----------------------------------------------------------------------------
set(GA_FILE_IO_CONTRACT_HEADERS
    include/GAFileTypes.h
    include/IGAFileReader.h
    include/IGAFileWriter.h
)

# -----------------------------------------------------------------------------
# FileIO service/registry/loader implementations; format-specific parsers belong in plugin directories.
# -----------------------------------------------------------------------------
set(GA_FILE_IO_SOURCES
    src/GAFileCodec.cpp
    src/GAFileCodecRegistry.cpp
    src/GAFileIOService.cpp
    src/GAFilePluginLoader.cpp
)


# Tests of the shared bridge and each independent runtime plugin.
set(GA_MESH_FILE_BRIDGE_TEST_SOURCES
    tests/GAMeshFileCodecBridgeTests.cpp
)
set(GA_MESH_FILE_CODEC_TEST_SOURCES
    tests/GAMeshFileCodecTests.cpp
)

# Service lifecycle/event tests use a fake codec and no vendor parser.
set(GA_FILE_IO_SERVICE_TEST_SOURCES
    tests/GAFileIOServiceTests.cpp
)

set(GA_FILE_FAILURE_PLUGIN_SOURCES
    tests/GAFileFailurePlugin.cpp
)
set(GA_FILE_PLUGIN_LOADER_TEST_SOURCES
    tests/GAFilePluginLoaderTests.cpp
)
