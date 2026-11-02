#pragma once

#include "GADataTypes.h"
#include "IGAObject.h"

#include <memory>
#include <string>

class GADataStore;

/**
 * @brief Base class for data whose lifetime is managed by GADataStore.
 *
 * The store assigns the object ID. Derived classes call touch() after every
 * successful content mutation so caches can detect stale derived resources.
 *
 */
class GADataObject : public IGAObject
{
public:
    ~GADataObject() override;

    [[nodiscard]] GADataObjectId id() const noexcept;
    [[nodiscard]] GADataRevision revision() const noexcept;
    [[nodiscard]] GADataObjectType type() const noexcept;
    [[nodiscard]] const std::string& name() const noexcept;

    void setName(std::string name);

protected:
    GADataObject(GADataObjectType type, std::string name);

    /// Advances the content revision after a successful mutation.
    void touch() noexcept;

private:
    friend class GADataStore;
    void assignId(GADataObjectId id) noexcept;

    class Impl;
    std::unique_ptr<Impl> impl_;
};
