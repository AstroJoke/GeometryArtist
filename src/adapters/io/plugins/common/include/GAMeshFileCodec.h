#pragma once

#include "GAFileCodec.h"
#include "IGAMeshFileBackend.h"

#include <memory>

/**
 * @brief Adapts a stream backend to GA's file-service and data-store contracts.
 *
 * Owns filesystem transactions and data publication, not format algorithms.
 * The plugin factory injects a backend; adding one does not change this class.
 * Derived codecs supply only format metadata and backend construction.
 *
 */
class GAMeshFileCodec : public GAFileCodec
{
public:
    ~GAMeshFileCodec() override;
    [[nodiscard]] bool acceptsExtension(const char* extension) const noexcept override;
    [[nodiscard]] GAFileResult read(const GAFileReadRequest& request, GADataStore& store) override;
    [[nodiscard]] GAFileResult write(const GAFileWriteRequest& request, const GADataStore& store) override;

protected:
    explicit GAMeshFileCodec(std::unique_ptr<IGAMeshFileBackend> backend);

private:
    std::unique_ptr<IGAMeshFileBackend> backend_;
};
