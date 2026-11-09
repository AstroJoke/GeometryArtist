#pragma once

#include "GAMeshFileCodec.h"

class GAOffFileCodec : public GAMeshFileCodec
{
public:
    GAOffFileCodec();

    [[nodiscard]] const GAFileFormatDescriptor& descriptor() const noexcept override;
};
