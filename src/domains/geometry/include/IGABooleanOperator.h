#pragma once

#include "GAGeometryTypes.h"

/**
 * @brief Geometry-kernel port for Boolean set operations.
 *
 * Inputs are borrowed and remain valid after the call. A successful output is a
 * new kernel-owned geometry handle.
 */
class IGABooleanOperator
{
public:
    virtual ~IGABooleanOperator() = default;

    [[nodiscard]] virtual GAGeometryResult apply(GABooleanOperation operation, GAGeometryHandle left,
                                                 GAGeometryHandle right, const GAGeometryOptions& options) = 0;
};
