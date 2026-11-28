#pragma once

#include "GADataTypes.h"
#include "GAGeometryTypes.h"

class GADataObject;
class GADataStore;

/**
 * @brief Adapter boundary between neutral GA data and kernel-native geometry.
 *
 * Import creates a kernel-owned working object; export creates a neutral data
 * object in the destination store. This separates persistent/interchange data
 * from CGAL, OpenCascade, or another kernel's internal representation.
 */
class IGAGeometryResource
{
public:
    virtual ~IGAGeometryResource() = default;

    /// Creates a kernel-owned object from borrowed neutral data.
    [[nodiscard]] virtual GAGeometryHandle importGeometry(const GADataObject& source) = 0;
    [[nodiscard]] virtual GADataHandle exportGeometry(GAGeometryHandle geometry, const char* name,
                                                      GADataStore& destination) const = 0;
    /// Releases a handle created by this same resource instance.
    virtual void releaseGeometry(GAGeometryHandle geometry) noexcept = 0;
    [[nodiscard]] virtual bool isValid(GAGeometryHandle geometry) const noexcept = 0;
    [[nodiscard]] virtual GAGeometryRepresentation representation(GAGeometryHandle geometry) const noexcept = 0;
};
