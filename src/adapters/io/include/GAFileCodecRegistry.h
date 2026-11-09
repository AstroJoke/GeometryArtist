#pragma once

#include "GAFileCodec.h"

#include <cstddef>
#include <memory>

/**
 * @brief Thread-safe, non-owning index of available file codecs.
 *
 * Lookup may use an explicit format ID or a file-name extension. Registration
 * rejects duplicate backend/format pairs. Unqualified lookups require a unique match.
 *
 */
class GAFileCodecRegistry
{
public:
    GAFileCodecRegistry();
    ~GAFileCodecRegistry();

    GAFileCodecRegistry(const GAFileCodecRegistry&) = delete;
    GAFileCodecRegistry& operator=(const GAFileCodecRegistry&) = delete;

    /// Borrows codec; its plugin loader must outlive the registration.
    [[nodiscard]] bool registerCodec(GAFileCodec* codec);
    [[nodiscard]] bool unregisterCodec(const char* formatId, const char* backendId = nullptr) noexcept;
    [[nodiscard]] GAFileCodec* findById(const char* formatId, const char* backendId = nullptr) const noexcept;
    [[nodiscard]] GAFileCodec* findByPath(const char* filePath, const char* backendId = nullptr) const noexcept;
    struct Match
    {
        GAFileCodec* codec = nullptr;
        GAFileStatus status = GAFileStatus::FormatNotFound;
    };

    // Lookup and capability validation happen under one registry lock.
    [[nodiscard]] Match resolve(const GAFileCodecQuery& query) const noexcept;
    [[nodiscard]] std::vector<GAFileCodecInfo> queryCodecs(const GAFileCodecQuery& query = {}) const;
    [[nodiscard]] std::size_t size() const noexcept;
    void clear() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
