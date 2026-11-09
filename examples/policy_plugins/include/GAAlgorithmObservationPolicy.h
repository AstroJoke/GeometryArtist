#pragma once

#include "GAAlgorithmTypes.h"

namespace ga::example
{
class IGAAlgorithmObserver
{
public:
    virtual ~IGAAlgorithmObserver() = default;
    virtual void onEvent(GAEvent event, GAStatus status) noexcept = 0;
};

class GAAlgorithmObservationPolicy
{
public:
    virtual ~GAAlgorithmObservationPolicy() = default;
    // The observer is borrowed until replaced, cleared or shutdown. Calls are synchronous.
    // Do not reenter or destroy the component in a callback.
    virtual void setObserver(IGAAlgorithmObserver& observer) noexcept = 0;
    virtual void clearObserver() noexcept = 0;
};
} // namespace ga::example
