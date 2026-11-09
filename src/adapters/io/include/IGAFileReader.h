#pragma once

#include "GAFileTypes.h"

class GADataStore;

/**
 * @brief Narrow input port used by import use cases.
 */
class IGAFileReader
{
public:
    virtual ~IGAFileReader() = default;

    [[nodiscard]] virtual GAFileResult read(const GAFileReadRequest& request, GADataStore& store) = 0;
};
