#pragma once

#include "GAGeometryTypes.h"
#include "GAWorkbenchTypes.h"

#include <string>

class GADataStore;
class IGAArtifactRepository;
class IGABooleanOperator;
class IGAGeometryResource;

struct GABooleanUseCaseResult
{
    GAGeometryStatus status = GAGeometryStatus::Failed;
    GAArtifactId artifact = GAInvalidArtifactId;
    std::string message;
};

/**
 * @brief Application orchestration for one geometry Boolean operation.
 *
 * The use case resolves artifacts, imports neutral data into the selected kernel,
 * executes the operation, exports the result, and publishes a new artifact. All
 * kernel-specific behavior remains behind injected ports.
 */
class GABooleanUseCase
{
public:
    GABooleanUseCase(GADataStore& data, IGAArtifactRepository& artifacts, IGAGeometryResource& resources,
                     IGABooleanOperator& booleanOperator) noexcept;

    [[nodiscard]] GABooleanUseCaseResult execute(GABooleanOperation operation, GAArtifactId left, GAArtifactId right,
                                                 const GAGeometryOptions& options = {});

private:
    GADataStore& data_;
    IGAArtifactRepository& artifacts_;
    IGAGeometryResource& resources_;
    IGABooleanOperator& booleanOperator_;
};
