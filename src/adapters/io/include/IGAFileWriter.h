#pragma once

#include "GAFileTypes.h"

class GADataStore;

/**
 * @brief Narrow output port used by export use cases.
 *
 */
class IGAFileWriter
{
public:
    virtual ~IGAFileWriter() = default;

    [[nodiscard]] virtual GAFileResult write(const GAFileWriteRequest& request, const GADataStore& store) = 0;
};
