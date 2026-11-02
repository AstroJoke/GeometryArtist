#pragma once

/**
 * @brief Non-copyable polymorphic base for objects crossing GA module boundaries.
 *
 */
class IGAObject
{
public:
    IGAObject() = default;
    virtual ~IGAObject() = default;

    IGAObject(const IGAObject&) = delete;
    IGAObject& operator=(const IGAObject&) = delete;
    IGAObject(IGAObject&&) = delete;
    IGAObject& operator=(IGAObject&&) = delete;
};
