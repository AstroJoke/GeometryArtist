#pragma once

#include "GAGeometryInputPrecision.h"
#include "IGABooleanOperator.h"
#include "IGAGeometryResource.h"

#include <cstddef>
#include <memory>

// Operations borrow inputs and publish independent exact geometry handles.
class GACgalGeometryAdapter : public IGAGeometryResource, public IGABooleanOperator
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
    [[nodiscard]] std::size_t resourceCount() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
