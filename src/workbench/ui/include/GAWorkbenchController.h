#pragma once

#include "GAWorkbenchTypes.h"

struct GAFileReadRequest;
class GAExportUseCase;
class GAImportUseCase;
class IGAWorkbenchView;

class GAWorkbenchController
{
public:
    GAWorkbenchController(GAImportUseCase& importer, GAExportUseCase& exporter, IGAWorkbenchView& view) noexcept;

    [[nodiscard]] bool openDocument(const GAFileReadRequest& request);
    [[nodiscard]] bool openDocument(const char* filePath, const char* formatId = nullptr);
    [[nodiscard]] bool saveDocument(GAArtifactId artifact, const char* filePath, const char* formatId = nullptr,
                                    bool binary = false, const char* backendId = nullptr);

private:
    GAImportUseCase& importer_;
    GAExportUseCase& exporter_;
    IGAWorkbenchView& view_;
};
