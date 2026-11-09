#pragma once

#include "GAFileTypes.h"
#include "GAMeshDataObject.h"

#include <iosfwd>
#include <memory>

struct GAMeshDecodeOptions
{
    bool triangulate = true;
    bool generateNormals = false;
};

struct GAMeshDecodeResult
{
    GAFileStatus status = GAFileStatus::ParseFailed;
    std::unique_ptr<GAMeshDataObject> object;
    const char* message = nullptr;
};

struct GAMeshEncodeResult
{
    GAFileStatus status = GAFileStatus::WriteFailed;
    const char* message = nullptr;
};

/**
 * @brief Plugin-internal port for translating streams and neutral mesh data.
 *
 * Backends do not open paths, register handles, create workbench artifacts, or
 * retain borrowed views. A successful decode transfers one complete GA object.
 * Messages must remain valid while the backend is alive. This is a C++ port
 * inside a plugin, not a new cross-compiler binary ABI.
 *
 */
class IGAMeshFileBackend
{
public:
    virtual ~IGAMeshFileBackend() = default;
    [[nodiscard]] virtual GAMeshDecodeResult decode(std::istream& input, const GAMeshDecodeOptions& options) = 0;
    [[nodiscard]] virtual GAMeshEncodeResult encode(std::ostream& output, const GAMeshDataView& mesh, bool binary) = 0;
};
