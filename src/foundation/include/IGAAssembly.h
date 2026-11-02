#pragma once

#include "IGARootObject.h"

/**
 * @brief Lifecycle contract for a replaceable GA component assembly.
 *
 * An assembly groups related capabilities behind one initialization boundary.
 * Calls requiring resources are valid only after init() succeeds and before
 * shutdown(). shutdown() must be safe to call repeatedly.
 *
 */
template <class... TPolicies> class IGAAssembly : public IGARootObject<TPolicies...>
{
public:
    ~IGAAssembly() override = default;

    virtual bool init() = 0;
    virtual void shutdown() noexcept = 0;
    [[nodiscard]] virtual bool isInitialized() const noexcept = 0;

protected:
    IGAAssembly() = default;
};
