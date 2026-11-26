#include "GAFileUseCases.h"

#include "GADataObject.h"
#include "GADataStore.h"
#include "IGAArtifactRepository.h"
#include "IGAFileReader.h"
#include "IGAFileWriter.h"

namespace
{
GAArtifactType artifactTypeFor(GADataObjectType type) noexcept
{
    switch (type)
    {
    case GADataObjectType::TriangleMesh:
    case GADataObjectType::PolygonMesh:
        return GAArtifactType::Mesh;
    case GADataObjectType::PointCloud:
    case GADataObjectType::Polyline:
        return GAArtifactType::Geometry;
    case GADataObjectType::Image:
    case GADataObjectType::Volume:
        return GAArtifactType::Image;
    case GADataObjectType::SimulationField:
        return GAArtifactType::SimulationField;
    case GADataObjectType::Layout:
        return GAArtifactType::Layout;
    case GADataObjectType::Table:
        return GAArtifactType::Table;
    default:
        return GAArtifactType::Binary;
    }
}
} // namespace

GAImportUseCase::GAImportUseCase(IGAFileReader& reader, GADataStore& data, IGAArtifactRepository& artifacts) noexcept
    : reader_(reader)
    , data_(data)
    , artifacts_(artifacts)
{
}

GAImportResult GAImportUseCase::execute(const char* filePath, const char* formatId)
{
    return execute(GAFileReadRequest{filePath, formatId});
}

GAImportResult GAImportUseCase::execute(const GAFileReadRequest& request)
{
    if (request.filePath == nullptr || *request.filePath == '\0')
    {
        return {GAFileStatus::InvalidArgument, GAInvalidArtifactId, "A file path is required"};
    }

    const GAFileResult result = reader_.read(request, data_);
    if (result.status != GAFileStatus::Success)
    {
        return {result.status, GAInvalidArtifactId, result.message ? result.message : "Import failed"};
    }

    const auto object = data_.access(result.data);
    if (!object)
    {
        return {GAFileStatus::ParseFailed, GAInvalidArtifactId, "The reader returned an unknown data handle"};
    }

    const std::string_view name = object->name().empty() ? std::string_view(request.filePath) : object->name();
    try
    {
        const auto artifact = artifacts_.create(artifactTypeFor(object->type()), name,
                                                "application/x-geometry-artist-data", result.data);
        if (artifact != GAInvalidArtifactId)
        {
            return {GAFileStatus::Success, artifact, {}};
        }
    }
    catch (...)
    {
        (void)data_.release(result.data);
        throw;
    }
    (void)data_.release(result.data);
    return {GAFileStatus::ParseFailed, GAInvalidArtifactId, "The workbench could not publish the imported data"};
}

GAExportUseCase::GAExportUseCase(IGAFileWriter& writer, const GADataStore& data,
                                 const IGAArtifactRepository& artifacts) noexcept
    : writer_(writer)
    , data_(data)
    , artifacts_(artifacts)
{
}

GAFileResult GAExportUseCase::execute(GAArtifactId artifact, const char* filePath, const char* formatId, bool binary,
                                     const char* backendId)
{
    if (filePath == nullptr || *filePath == '\0')
    {
        return {GAFileStatus::InvalidArgument, GAInvalidDataHandle, 0, "An output path is required"};
    }
    const GADataHandle input = artifacts_.dataHandle(artifact);
    if (!data_.contains(input))
    {
        return {GAFileStatus::InvalidArgument, GAInvalidDataHandle, 0, "The artifact has no available data"};
    }
    return writer_.write(GAFileWriteRequest{filePath, formatId, input, binary, backendId}, data_);
}
