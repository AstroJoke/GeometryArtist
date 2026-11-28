#pragma once

#include "GAGeometryTypes.h"

class IGATransformOperator
{
public:
    virtual ~IGATransformOperator() = default;

    // Legacy in-place operation; adapters document their failure atomicity.
    [[nodiscard]] virtual GAGeometryStatus transform(GAGeometryHandle geometry, const GATransform& transform) = 0;
    // Borrow input and return a new resource; default Unsupported keeps legacy implementations
    // source-compatible.
    [[nodiscard]] virtual GAGeometryResult createTransformed(GAGeometryHandle, const GATransform&)
    {
        return {GAGeometryStatus::Unsupported, GAInvalidGeometryHandle, "Non-mutating transform is not implemented"};
    }
};
