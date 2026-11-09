#pragma once

#include "GADataObject.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

/**
 * @brief Owning neutral representation of an indexed triangle mesh.
 *
 * setData() validates and copies the supplied non-owning buffers. Consequently,
 * callers may release their source buffers immediately after the call returns.
 *
 */
class GAMeshDataObject : public GADataObject
{
public:
    explicit GAMeshDataObject(std::string name = std::string());
    ~GAMeshDataObject() override;

    GAMeshDataObject(const GAMeshDataObject&) = delete;
    GAMeshDataObject& operator=(const GAMeshDataObject&) = delete;
    GAMeshDataObject(GAMeshDataObject&&) = delete;
    GAMeshDataObject& operator=(GAMeshDataObject&&) = delete;

    [[nodiscard]] bool setData(const GAMeshDataView& data);
    void clear() noexcept;

    /**
     * @brief Returns a view borrowed from this object.
     *
     * The view becomes invalid after mutation or destruction of this object.
     */
    [[nodiscard]] GAMeshDataView view() const noexcept;
    [[nodiscard]] std::size_t vertexCount() const noexcept;
    [[nodiscard]] std::size_t indexCount() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
