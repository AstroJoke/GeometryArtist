#pragma once

#include "GAMeshFileCodec.h"

class GAObjFileCodec : public GAMeshFileCodec
{
public:
    GAObjFileCodec();

    [[nodiscard]] const GAFileFormatDescriptor& descriptor() const noexcept override;
};
