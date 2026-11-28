#include "GAWorkbenchController.h"

#include "GAFileUseCases.h"
#include "IGAWorkbenchView.h"

GAWorkbenchController::GAWorkbenchController(GAImportUseCase& importer, GAExportUseCase& exporter,
                                             IGAWorkbenchView& view) noexcept
    : importer_(importer)
    , exporter_(exporter)
    , view_(view)
{
}

bool GAWorkbenchController::openDocument(const char* filePath, const char* formatId)
{
    return openDocument(GAFileReadRequest{filePath, formatId});
}

bool GAWorkbenchController::openDocument(const GAFileReadRequest& request)
{
    const GAImportResult result = importer_.execute(request);
    if (result.status != GAFileStatus::Success)
    {
        view_.showError(result.message.c_str());
        return false;
    }
    view_.setActiveArtifact(result.artifact);
    view_.showStatus("Document imported");
    return true;
}

bool GAWorkbenchController::saveDocument(GAArtifactId artifact, const char* filePath, const char* formatId, bool binary,
                                         const char* backendId)
{
    const GAFileResult result = exporter_.execute(artifact, filePath, formatId, binary, backendId);
    if (result.status != GAFileStatus::Success)
    {
        view_.showError(result.message != nullptr ? result.message : "Export failed");
        return false;
    }
    view_.showStatus("Document exported");
    return true;
}
