#pragma once

#include "GAFileTypes.h"

class IGAFileObserver
{
public:
    virtual ~IGAFileObserver() = default;
    virtual void onEvent(const GAFileEvent& event) noexcept = 0;
};

/**
 * @brief Installs one non-owning observer for file-operation events.
 *
 * Callbacks may overlap. After clearing or replacing an observer, keep it alive
 * until active callbacks finish. clearObserver() does not wait for them.
 *
 */
class GAFileObservationPolicy
{
public:
    virtual ~GAFileObservationPolicy() = default;

    virtual void setObserver(IGAFileObserver& observer) noexcept = 0;
    virtual void clearObserver() noexcept = 0;
};
