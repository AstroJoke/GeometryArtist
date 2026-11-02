#pragma once

#include "GADataObject.h"

#include <cstddef>
#include <memory>

/**
 * @brief Thread-safe owner and lookup service for neutral GA data objects.
 *
 * Registration transfers ownership into the store. access() returns a shared
 * lifetime lease: an object already obtained by a caller remains alive even if
 * its handle is later released or replaced in the store.
 *
 *
 * @note Container access is synchronized, but concurrent mutation of the same
 *       GADataObject must be coordinated by its callers.
 */
class GADataStore final
{
public:

    GADataStore();
    ~GADataStore();

    GADataStore(const GADataStore&) = delete;
    GADataStore& operator=(const GADataStore&) = delete;

    /**
     * @brief Transfers an object into the store and returns its stable handle.
     * @return GAInvalidDataHandle when object is null.
     *
     */
    [[nodiscard]] GADataHandle registe(std::unique_ptr<GADataObject> object);

    /// Acquires a read-only shared lifetime lease.
    [[nodiscard]] std::shared_ptr<const GADataObject> access(GADataHandle handle) const noexcept;

    /// Acquires a mutable shared lifetime lease; it does not lock object content.
    [[nodiscard]] std::shared_ptr<GADataObject> accessMutable(GADataHandle handle) noexcept;

    /// Replaces the stored object while preserving the handle and object ID.
    [[nodiscard]] bool replace(GADataHandle handle, std::unique_ptr<GADataObject> object);
    [[nodiscard]] bool release(GADataHandle handle) noexcept;
    [[nodiscard]] bool contains(GADataHandle handle) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    void clear() noexcept;

private:

    class Impl;
    std::unique_ptr<Impl> impl_;
};
