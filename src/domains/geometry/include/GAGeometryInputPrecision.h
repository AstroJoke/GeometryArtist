#pragma once
#include <cstdint>

// Zero preserves binary64 input exactly. Positive values select nearest integer/denominator, ties away from zero.
// Applied once to external input, never to native Boolean results or copied geometry.
struct GAGeometryInputPrecision
{
    std::uint32_t coordinateDenominator = 0;
    friend bool operator==(GAGeometryInputPrecision, GAGeometryInputPrecision) = default;
};
