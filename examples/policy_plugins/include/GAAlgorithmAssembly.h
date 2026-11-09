#pragma once

#include "GAAlgorithmObservationPolicy.h"
#include "GAAlgorithmPropertyPolicy.h"
#include "GAAlgorithmSerializationPolicy.h"
#include "IGAAssembly.h"

#include <span>
#include <vector>

namespace ga::example
{
class GAAlgorithmAssembly
    : public IGAAssembly<GAAlgorithmPropertyPolicy, GAAlgorithmSerializationPolicy, GAAlgorithmObservationPolicy>
{
public:
    // Calls are synchronous and externally serialized. Output changes only on success.
    [[nodiscard]] virtual GAStatus transform(GAOperation operation, std::span<const GAPoint> input,
                                             std::vector<GAPoint>& output) = 0;
};
} // namespace ga::example
