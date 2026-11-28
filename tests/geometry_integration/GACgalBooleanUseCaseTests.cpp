#include "GAArtifactStore.h"
#include "GABooleanUseCase.h"
#include "GACgalGeometryAdapter.h"
#include "GACgalGeometryTestMeshes.h"
#include "GADataStore.h"

#include <iostream>

using namespace ga::cgal_test;

int main()
{
    try
    {
        GADataStore data;
        GAArtifactStore artifacts;
        GACgalGeometryAdapter geometry;
        GABooleanUseCase useCase(data, artifacts, geometry, geometry);
        const auto leftData = data.registe(cube()), rightData = data.registe(cylinder());
        const auto left = artifacts.create(GAArtifactType::Mesh, "cube", "mesh", leftData);
        const auto right = artifacts.create(GAArtifactType::Mesh, "cylinder", "mesh", rightData);
        const auto output = useCase.execute(GABooleanOperation::Difference, left, right);
        require(output.status == GAGeometryStatus::Success && output.artifact != left && output.artifact != right,
                "Workbench Boolean did not publish a new artifact");
        const auto result =
            std::dynamic_pointer_cast<const GAMeshDataObject>(data.access(artifacts.dataHandle(output.artifact)));
        require(result && std::abs(signedVolume(*result) - (8 - 2 * cylinderArea())) < VolumeTolerance,
                "Workbench result volume mismatch");
        require(data.size() == 3 && artifacts.size() == 3 && geometry.resourceCount() == 0,
                "Workbench ownership counts mismatch");
        const auto input = std::dynamic_pointer_cast<const GAMeshDataObject>(data.access(leftData));
        require(input && std::abs(signedVolume(*input) - 8) < VolumeTolerance, "Workbench changed an input");
        GAGeometryOptions unsupported;
        unsupported.repairInput = true;
        const auto rejected = useCase.execute(GABooleanOperation::Union, left, right, unsupported);
        require(rejected.status == GAGeometryStatus::Unsupported && rejected.artifact == GAInvalidArtifactId,
                "Workbench failure was published");
        require(data.size() == 3 && artifacts.size() == 3 && geometry.resourceCount() == 0,
                "Workbench failure leaked native resources");
        const auto empty = useCase.execute(GABooleanOperation::Difference, left, left);
        require(empty.status == GAGeometryStatus::Success && geometry.resourceCount() == 0,
                "Workbench empty result failed");
        const auto emptyMesh =
            std::dynamic_pointer_cast<const GAMeshDataObject>(data.access(artifacts.dataHandle(empty.artifact)));
        require(emptyMesh && emptyMesh->empty(), "Workbench did not publish the empty result");
        std::cout << "Real CGAL / GABooleanUseCase integration passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
