#pragma once

#include "GAMeshFileCodec.h"

class GAPlyFileCodec : public GAMeshFileCodec
{
public:
    GAPlyFileCodec();

    [[nodiscard]] const GAFileFormatDescriptor& descriptor() const noexcept override;
};
