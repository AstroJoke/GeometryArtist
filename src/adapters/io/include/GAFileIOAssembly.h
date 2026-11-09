#pragma once

#include "GAFileCodecPolicy.h"
#include "GAFileExecutionPolicy.h"
#include "GAFileObservationPolicy.h"
#include "GAFilePropertyPolicy.h"
#include "GAFileTypes.h"
#include "IGAAssembly.h"
#include "IGAFileReader.h"
#include "IGAFileWriter.h"

class GADataStore;

/**
 * @brief Complete replaceable contract for the file-I/O subsystem.
 *
 * The assembly composes focused policies and the narrow reader/writer ports. It
 * defines a plugin-facing boundary without exposing Qt or a concrete file format.
 *
 */
class GAFileIOAssembly
    : public IGAAssembly<GAFilePropertyPolicy, GAFileCodecPolicy, GAFileObservationPolicy, GAFileExecutionPolicy>,
      public IGAFileReader,
      public IGAFileWriter
{
public:
    ~GAFileIOAssembly() override = default;

    [[nodiscard]] virtual GAFileResult read(const GAFileReadRequest& request, GADataStore& store) override = 0;
    [[nodiscard]] virtual GAFileResult write(const GAFileWriteRequest& request, const GADataStore& store) override = 0;

protected:
    GAFileIOAssembly() = default;
};
