#pragma once

#include "GADataStore.h"
#include "GADataTypes.h"

/**
 * @brief Port for converting one neutral GA data representation into another.
 *
 * A converter is different from a file codec: it transforms in-memory data,
 * while a codec crosses the external file boundary.
 *
 */
class IGADataConverter
{
public:
    virtual ~IGADataConverter();

    [[nodiscard]] virtual const char* converterId() const noexcept = 0;
    [[nodiscard]] virtual GADataObjectType sourceType() const noexcept = 0;
    [[nodiscard]] virtual GADataObjectType targetType() const noexcept = 0;
    [[nodiscard]] virtual bool canConvert(const GADataObject& source) const noexcept = 0;

    /// Converts source and registers the produced object in store.
    [[nodiscard]] virtual GAConversionResult convert(const GADataObject& source, const GAConversionOptions& options,
                                                     GADataStore& store) = 0;
};
