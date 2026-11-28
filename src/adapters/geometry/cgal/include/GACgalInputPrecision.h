#pragma once
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace ga::cgal_precision
{
inline constexpr double MaximumExactDoubleInteger = 9'007'199'254'740'991.0;
// Construct the rational from an exactly representable integer. Do not divide in double first.
// Number must implement exact construction from binary64 and rational division.
template <class Number> Number inputNumber(double value, std::uint32_t denominator)
{
    if (!std::isfinite(value))
        throw std::invalid_argument("Non-finite input coordinate");
    if (denominator == 0)
        return Number(value);
    const double scaled = value * static_cast<double>(denominator);
    if (!std::isfinite(scaled) || std::abs(scaled) > MaximumExactDoubleInteger)
        throw std::invalid_argument("Input exceeds the supported quantization range");
    return Number(std::round(scaled)) / Number(static_cast<double>(denominator));
}
} // namespace ga::cgal_precision
