#pragma once

#include "GADataStore.h"
#include "GAFileTypes.h"

/**
 * @brief Format-specific translator between an external file and GADataStore.
 *
 * @note Owned by its plugin loader. Registration in a service does not transfer ownership.
 */
class GAFileCodec
{
public:
    virtual ~GAFileCodec();

    /// Returns metadata whose lifetime is tied to this codec.
    [[nodiscard]] virtual const GAFileFormatDescriptor& descriptor() const noexcept = 0;
    [[nodiscard]] virtual bool acceptsExtension(const char* extension) const noexcept = 0;

    /// Decodes a file and registers the resulting neutral object in store.
    [[nodiscard]] virtual GAFileResult read(const GAFileReadRequest& request, GADataStore& store) = 0;

    /// Encodes one object borrowed from store into an external file.
    [[nodiscard]] virtual GAFileResult write(const GAFileWriteRequest& request, const GADataStore& store) = 0;
};
