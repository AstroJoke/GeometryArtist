#pragma once

#include "GAFileTypes.h"
#include "GAWorkbenchTypes.h"

#include <string>

class GADataStore;
class IGAArtifactRepository;
class IGAFileReader;
class IGAFileWriter;

struct GAImportResult
{
    GAFileStatus status = GAFileStatus::Unsupported;
    GAArtifactId artifact = GAInvalidArtifactId;
    std::string message;
};

class GAImportUseCase
{
public:
    GAImportUseCase(IGAFileReader& reader, GADataStore& data, IGAArtifactRepository& artifacts) noexcept;

    [[nodiscard]] GAImportResult execute(const GAFileReadRequest& request);
    [[nodiscard]] GAImportResult execute(const char* filePath, const char* formatId = nullptr);

private:
    IGAFileReader& reader_;
    GADataStore& data_;
    IGAArtifactRepository& artifacts_;
};

class GAExportUseCase
{
public:
    GAExportUseCase(IGAFileWriter& writer, const GADataStore& data, const IGAArtifactRepository& artifacts) noexcept;

    [[nodiscard]] GAFileResult execute(GAArtifactId artifact, const char* filePath, const char* formatId = nullptr,
                                       bool binary = false, const char* backendId = nullptr);

private:
    IGAFileWriter& writer_;
    const GADataStore& data_;
    const IGAArtifactRepository& artifacts_;
};
