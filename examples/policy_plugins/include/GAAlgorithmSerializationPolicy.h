#pragma once

#include <string>
#include <string_view>

namespace ga::example
{
class GAAlgorithmSerializationPolicy
{
public:
    virtual ~GAAlgorithmSerializationPolicy() = default;
    // This example persists settings only. The caller chooses where to store the text.
    [[nodiscard]] virtual std::string saveSettings() const = 0;
    // Invalid input leaves the current settings unchanged.
    virtual bool restoreSettings(std::string_view text) = 0;
};
} // namespace ga::example
