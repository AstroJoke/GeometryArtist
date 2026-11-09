#pragma once

#include "GAFileCodec.h"

#include <cstddef>

/**
 * @brief Capability for discovering format codecs through an assembly.
 *
 * This policy is non-owning: it exposes registration and lookup, while plugin
 * loaders retain codec ownership.
 *
 */
class GAFileCodecPolicy
{
public:
    virtual ~GAFileCodecPolicy() = default;

    [[nodiscard]] virtual bool registerCodec(GAFileCodec* codec) = 0;
    [[nodiscard]] virtual bool unregisterCodec(const char* formatId, const char* backendId = nullptr) noexcept = 0;
    [[nodiscard]] virtual const GAFileCodec* codec(const char* formatId,
                                                   const char* backendId = nullptr) const noexcept = 0;
    [[nodiscard]] virtual std::size_t codecCount() const noexcept = 0;

    // An empty query lists registered implementations; filters narrow their advertised capabilities.
    [[nodiscard]] virtual std::vector<GAFileCodecInfo> queryCodecs(const GAFileCodecQuery& query = {}) const = 0;
};
