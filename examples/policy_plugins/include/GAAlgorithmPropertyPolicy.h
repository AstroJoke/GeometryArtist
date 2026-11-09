#pragma once

#include "GAAlgorithmTypes.h"

namespace ga::example
{
class GAAlgorithmPropertyPolicy
{
public:
    virtual ~GAAlgorithmPropertyPolicy() = default;
    // Returned names are borrowed from the plugin.
    [[nodiscard]] virtual GAEngineInfo engineInfo() const noexcept = 0;
    [[nodiscard]] virtual bool supports(GAOperation operation) const noexcept = 0;
    [[nodiscard]] virtual GAAlgorithmSettings settings() const noexcept = 0;
    virtual bool setSettings(const GAAlgorithmSettings& settings) noexcept = 0;
};
} // namespace ga::example
