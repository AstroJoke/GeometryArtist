#pragma once

#include "GAGeometryTypes.h"

#include <cstddef>

class IGAConvexHullOperator
{
public:
    virtual ~IGAConvexHullOperator() = default;

    // Borrow inputs and return a new hull; endpoint hulls are not general or rotational sweeps.
    [[nodiscard]] virtual GAGeometryResult computeHull(const GAGeometryHandle* geometries, std::size_t count,
                                                       const GAGeometryOptions& options) = 0;
};
