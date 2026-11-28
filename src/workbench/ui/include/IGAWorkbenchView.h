#pragma once

#include "GAWorkbenchTypes.h"

class IGAWorkbenchView
{
public:
    virtual ~IGAWorkbenchView() = default;

    virtual void setActiveArtifact(GAArtifactId artifact) = 0;
    // Messages are borrowed for the duration of the call.
    virtual void showStatus(const char* message) = 0;
    virtual void showError(const char* message) = 0;
};
