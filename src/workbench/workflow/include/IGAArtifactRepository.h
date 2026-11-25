#pragma once

#include "GAWorkbenchTypes.h"

#include <cstddef>
#include <string_view>

/**
 * @brief Persistence-agnostic port for workbench artifact metadata.
 *
 * Implementations own descriptors, not the data identified by their handles.
 */
class IGAArtifactRepository
{
public:
    virtual ~IGAArtifactRepository() = default;

    [[nodiscard]] virtual GAArtifactId create(GAArtifactType type, std::string_view name, std::string_view mediaType,
                                              GADataHandle data) = 0;
    virtual bool updateData(GAArtifactId artifact, GADataHandle data) = 0;
    [[nodiscard]] virtual bool descriptor(GAArtifactId artifact, GAArtifactDescriptor& output) const = 0;
    [[nodiscard]] virtual GADataHandle dataHandle(GAArtifactId artifact) const noexcept = 0;
    virtual bool remove(GAArtifactId artifact) noexcept = 0;
    [[nodiscard]] virtual std::size_t size() const noexcept = 0;
    virtual void clear() noexcept = 0;
};
