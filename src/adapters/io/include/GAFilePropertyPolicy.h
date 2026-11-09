#pragma once

#include <cstdint>

/**
 * @brief Runtime discovery and string-based configuration of a file service.
 *
 * String properties are suitable for plugin/workbench configuration boundaries;
 * strongly typed operation settings belong in GAFileReadRequest or
 * GAFileWriteRequest instead.
 *
 */
class GAFilePropertyPolicy
{
public:
    virtual ~GAFilePropertyPolicy() = default;

    [[nodiscard]] virtual const char* serviceName() const noexcept = 0;
    [[nodiscard]] virtual std::uint32_t serviceVersion() const noexcept = 0;
    virtual bool setProperty(const char* name, const char* value) = 0;
    [[nodiscard]] virtual const char* property(const char* name) const noexcept = 0;
};
