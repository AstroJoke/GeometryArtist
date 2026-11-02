#pragma once

#include "IGAObject.h"

#include <type_traits>

/**
 * @brief Composes a root interface from a fixed set of orthogonal policies.
 *
 * A policy represents one independent capability, such as properties,
 * observation, execution, or serialization. The template performs composition
 * only; it does not implement those capabilities.
 *
 *
 * @tparam TPolicies Public policy interfaces implemented by the derived assembly.
 */
template <class... TPolicies> class IGARootObject : public IGAObject, public TPolicies...
{
public:
    ~IGARootObject() override = default;

    template <class TPolicy>
        requires(std::is_same_v<TPolicy, TPolicies> || ...)
    /// Returns this object viewed through one of its declared policy interfaces.
    [[nodiscard]] TPolicy& policy() noexcept
    {
        return static_cast<TPolicy&>(*this);
    }

    template <class TPolicy>
        requires(std::is_same_v<TPolicy, TPolicies> || ...)
    /// Const overload of policy().
    [[nodiscard]] const TPolicy& policy() const noexcept
    {
        return static_cast<const TPolicy&>(*this);
    }

protected:
    IGARootObject() = default;
};
