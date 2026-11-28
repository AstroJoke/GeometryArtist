#pragma once

#include "GAGeometryInputPrecision.h"
#include "IGABooleanOperator.h"
#include "IGAConvexHullOperator.h"
#include "IGAGeometryResource.h"
#include "IGATransformOperator.h"

#include <cstddef>
#include <memory>

// Owns the exact solids referenced by this adapter's handles.
class GACgalGeometryAdapter : public IGAGeometryResource,
                              public IGABooleanOperator,
                              public IGATransformOperator,
                              public IGAConvexHullOperator
{
public:
    explicit GACgalGeometryAdapter(GAGeometryInputPrecision policy = {});
    ~GACgalGeometryAdapter() override;

    GACgalGeometryAdapter(const GACgalGeometryAdapter&) = delete;
    GACgalGeometryAdapter& operator=(const GACgalGeometryAdapter&) = delete;

    [[nodiscard]] GAGeometryHandle importGeometry(const GADataObject& source) override;
    [[nodiscard]] GADataHandle exportGeometry(GAGeometryHandle geometry, const char* name,
                                              GADataStore& destination) const override;
    void releaseGeometry(GAGeometryHandle geometry) noexcept override;
    [[nodiscard]] bool isValid(GAGeometryHandle geometry) const noexcept override;
    [[nodiscard]] GAGeometryRepresentation representation(GAGeometryHandle geometry) const noexcept override;
    [[nodiscard]] GAGeometryResult apply(GABooleanOperation operation, GAGeometryHandle left, GAGeometryHandle right,
                                         const GAGeometryOptions& options = {}) override;
    // Row-major matrix on column vectors; x'=m0*x+m1*y+m2*z+m3. Nonsingular affine transforms only.
    [[nodiscard]] GAGeometryStatus transform(GAGeometryHandle geometry, const GATransform& transform) override;
    [[nodiscard]] GAGeometryResult createTransformed(GAGeometryHandle geometry, const GATransform& transform) override;
    [[nodiscard]] GAGeometryResult computeHull(const GAGeometryHandle* geometries, std::size_t count,
                                               const GAGeometryOptions& options = {}) override;
    // The destination owns the copy. The source handle only needs to remain valid during the call.
    [[nodiscard]] GAGeometryResult copyGeometryFrom(const GACgalGeometryAdapter& source, GAGeometryHandle geometry);
    [[nodiscard]] std::size_t resourceCount() const noexcept;

private:
    friend class GACgalCoordinateAdapter;
    class Impl;
    std::unique_ptr<Impl> impl_;
};
