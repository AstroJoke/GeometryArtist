#pragma once

#include "IGADataConverter.h"

#include <cstddef>
#include <memory>

/**
 * @brief Thread-safe registry of in-memory data converters.
 *
 * Shared ownership allows a converter returned by find() to remain alive after
 * concurrent unregistration.
 *
 */
class GADataConverterRegistry final
{
public:
    GADataConverterRegistry();
    ~GADataConverterRegistry();

    GADataConverterRegistry(const GADataConverterRegistry&) = delete;
    GADataConverterRegistry& operator=(const GADataConverterRegistry&) = delete;

    [[nodiscard]] bool registerConverter(std::shared_ptr<IGADataConverter> converter);
    [[nodiscard]] bool unregisterConverter(const char* converterId) noexcept;
    [[nodiscard]] std::shared_ptr<IGADataConverter> find(GADataObjectType source,
                                                         GADataObjectType target) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    void clear() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
