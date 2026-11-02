#include "GAMeshDataObject.h"

#include <algorithm>
#include <utility>
#include <vector>

class GAMeshDataObject::Impl
{
public:
    std::vector<double> positions;
    std::vector<std::uint32_t> indices;
    std::vector<double> normals;
    std::vector<double> textureCoordinates;
};

GAMeshDataObject::GAMeshDataObject(std::string name)
    : GADataObject(GADataObjectType::TriangleMesh, std::move(name))
    , impl_(std::make_unique<Impl>())
{
}

GAMeshDataObject::~GAMeshDataObject() = default;

bool GAMeshDataObject::setData(const GAMeshDataView& data)
{
    // Validate the complete borrowed view before changing owned state. A failed
    // update therefore leaves the current mesh unchanged.
    if ((data.vertexCount != 0 && data.positions == nullptr) || (data.indexCount != 0 && data.indices == nullptr) ||
        (data.normalCount != 0 && data.normals == nullptr) ||
        (data.textureCoordinateCount != 0 && data.textureCoordinates == nullptr))
    {
        return false;
    }

    if (data.indices != nullptr && std::any_of(data.indices, data.indices + data.indexCount,
                                               [vertexCount = data.vertexCount](std::uint32_t index)
                                               {
                                                   return index >= vertexCount;
                                               }))
    {
        return false;
    }

    // Copying here establishes a clear ownership boundary between importers,
    // algorithm kernels, and the neutral data model.
    impl_->positions.clear();
    impl_->indices.clear();
    impl_->normals.clear();
    impl_->textureCoordinates.clear();
    if (data.vertexCount != 0)
        impl_->positions.assign(data.positions, data.positions + data.vertexCount * 3);
    if (data.indexCount != 0)
        impl_->indices.assign(data.indices, data.indices + data.indexCount);
    if (data.normalCount != 0)
        impl_->normals.assign(data.normals, data.normals + data.normalCount * 3);
    if (data.textureCoordinateCount != 0)
        impl_->textureCoordinates.assign(data.textureCoordinates,
                                         data.textureCoordinates + data.textureCoordinateCount * 2);
    touch();
    return true;
}

void GAMeshDataObject::clear() noexcept
{
    impl_->positions.clear();
    impl_->indices.clear();
    impl_->normals.clear();
    impl_->textureCoordinates.clear();
    touch();
}

GAMeshDataView GAMeshDataObject::view() const noexcept
{
    return GAMeshDataView{impl_->positions.data(),
                          impl_->positions.size() / 3,
                          impl_->indices.data(),
                          impl_->indices.size(),
                          impl_->normals.data(),
                          impl_->normals.size() / 3,
                          impl_->textureCoordinates.data(),
                          impl_->textureCoordinates.size() / 2};
}

std::size_t GAMeshDataObject::vertexCount() const noexcept
{
    return impl_->positions.size() / 3;
}

std::size_t GAMeshDataObject::indexCount() const noexcept
{
    return impl_->indices.size();
}

bool GAMeshDataObject::empty() const noexcept
{
    return impl_->positions.empty();
}
