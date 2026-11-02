#include "GADataObject.h"

#include <utility>

class GADataObject::Impl
{
public:
    Impl(GADataObjectType objectType, std::string objectName)
        : type(objectType)
        , name(std::move(objectName))
    {
    }

    GADataObjectId id = GAInvalidDataObjectId;
    GADataRevision revision = 1;
    GADataObjectType type = GADataObjectType::Unknown;
    std::string name;
};

GADataObject::GADataObject(GADataObjectType type, std::string name)
    : impl_(std::make_unique<Impl>(type, std::move(name)))
{
}

GADataObject::~GADataObject() = default;

GADataObjectId GADataObject::id() const noexcept
{
    return impl_->id;
}

GADataRevision GADataObject::revision() const noexcept
{
    return impl_->revision;
}

GADataObjectType GADataObject::type() const noexcept
{
    return impl_->type;
}

const std::string& GADataObject::name() const noexcept
{
    return impl_->name;
}

void GADataObject::setName(std::string name)
{
    impl_->name = std::move(name);
    touch();
}

void GADataObject::touch() noexcept
{
    ++impl_->revision;
}

void GADataObject::assignId(GADataObjectId id) noexcept
{
    impl_->id = id;
}
