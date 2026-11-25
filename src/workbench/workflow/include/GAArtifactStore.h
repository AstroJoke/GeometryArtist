#pragma once

#include "IGAArtifactRepository.h"

#include <cstddef>
#include <memory>
#include <string_view>

/**
 * @brief Thread-safe in-memory repository for artifact metadata.
 *
 * GADataStore remains the single owner of data. Removing an artifact therefore
 * does not release its GADataObject, and releasing data does not automatically
 * remove an artifact that references it.
 *
 */
class GAArtifactStore : public IGAArtifactRepository
{
public:
    GAArtifactStore();
    ~GAArtifactStore() override;

    GAArtifactStore(const GAArtifactStore&) = delete;
    GAArtifactStore& operator=(const GAArtifactStore&) = delete;

    [[nodiscard]] GAArtifactId create(GAArtifactType type, std::string_view name, std::string_view mediaType,
                                      GADataHandle data) override;
    bool updateData(GAArtifactId artifact, GADataHandle data) override;
    [[nodiscard]] bool descriptor(GAArtifactId artifact, GAArtifactDescriptor& output) const override;
    [[nodiscard]] GADataHandle dataHandle(GAArtifactId artifact) const noexcept override;
    bool remove(GAArtifactId artifact) noexcept override;
    [[nodiscard]] std::size_t size() const noexcept override;
    void clear() noexcept override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
