#pragma once

#include "GAMeshFileCodec.h"

class GAStlFileCodec : public GAMeshFileCodec
{
public:
    GAStlFileCodec();

    [[nodiscard]] const GAFileFormatDescriptor& descriptor() const noexcept override;
};
