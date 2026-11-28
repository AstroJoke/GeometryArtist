#include "GABooleanUseCase.h"

#include "GADataObject.h"
#include "GADataStore.h"
#include "IGAArtifactRepository.h"
#include "IGABooleanOperator.h"
#include "IGAGeometryResource.h"

namespace
{
struct TemporaryGeometry
{
    IGAGeometryResource& resources;
    GAGeometryHandle left = GAInvalidGeometryHandle;
    GAGeometryHandle right = GAInvalidGeometryHandle;
    GAGeometryHandle output = GAInvalidGeometryHandle;

    ~TemporaryGeometry()
    {
        if (output != GAInvalidGeometryHandle && output != left && output != right)
            resources.releaseGeometry(output);
        if (right != GAInvalidGeometryHandle && right != left)
            resources.releaseGeometry(right);
        if (left != GAInvalidGeometryHandle)
            resources.releaseGeometry(left);
    }
};

GAArtifactType artifactTypeFor(GADataObjectType type) noexcept
{
    switch (type)
    {
    case GADataObjectType::TriangleMesh:
    case GADataObjectType::PolygonMesh:
        return GAArtifactType::Mesh;
    case GADataObjectType::Image:
    case GADataObjectType::Volume:
        return GAArtifactType::Image;
    case GADataObjectType::SimulationField:
        return GAArtifactType::SimulationField;
    case GADataObjectType::Table:
        return GAArtifactType::Table;
    default:
        return GAArtifactType::Geometry;
    }
}
} // namespace

GABooleanUseCase::GABooleanUseCase(GADataStore& data, IGAArtifactRepository& artifacts, IGAGeometryResource& resources,
                                   IGABooleanOperator& booleanOperator) noexcept
    : data_(data)
    , artifacts_(artifacts)
    , resources_(resources)
    , booleanOperator_(booleanOperator)
{
}

GABooleanUseCaseResult GABooleanUseCase::execute(GABooleanOperation operation, GAArtifactId left, GAArtifactId right,
                                                 const GAGeometryOptions& options)
{
    const auto leftData = data_.access(artifacts_.dataHandle(left));
    const auto rightData = data_.access(artifacts_.dataHandle(right));
    if (!leftData || !rightData)
    {
        return {GAGeometryStatus::InvalidArgument, GAInvalidArtifactId,
                "One or both input artifacts do not reference live data"};
    }

    GADataHandle outputData = GAInvalidDataHandle;
    {
        TemporaryGeometry geometry{resources_};
        geometry.left = resources_.importGeometry(*leftData);
        if (geometry.left != GAInvalidGeometryHandle)
            geometry.right = resources_.importGeometry(*rightData);
        if (geometry.right == GAInvalidGeometryHandle)
            return {GAGeometryStatus::InvalidGeometry, GAInvalidArtifactId, "The geometry kernel could not import an input"};

        const auto result = booleanOperator_.apply(operation, geometry.left, geometry.right, options);
        if (result.output != GAInvalidGeometryHandle && resources_.isValid(result.output))
            geometry.output = result.output;
        if (result.status != GAGeometryStatus::Success)
            return {result.status, GAInvalidArtifactId, result.message ? result.message : "Boolean operation failed"};
        if (geometry.output == GAInvalidGeometryHandle)
            return {GAGeometryStatus::Failed, GAInvalidArtifactId, "The geometry kernel returned no live result"};

        outputData = resources_.exportGeometry(geometry.output, "Boolean result", data_);
    }

    const auto object = data_.access(outputData);
    if (!object)
        return {GAGeometryStatus::Failed, GAInvalidArtifactId, "The geometry kernel could not export its result"};

    try
    {
        const auto artifact = artifacts_.create(artifactTypeFor(object->type()), "Boolean result",
                                                "application/x-geometry-artist-data", outputData);
        if (artifact != GAInvalidArtifactId)
            return {GAGeometryStatus::Success, artifact, {}};
    }
    catch (...)
    {
        (void)data_.release(outputData);
        throw;
    }
    (void)data_.release(outputData);
    return {GAGeometryStatus::Failed, GAInvalidArtifactId, "The workbench could not publish the result artifact"};
}
