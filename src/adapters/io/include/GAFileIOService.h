#pragma once

#include "GAFileIOAssembly.h"

#include <cstddef>
#include <cstdint>
#include <memory>

/**
 * @brief Application-facing facade for file import and export.
 *
 * The service selects a registered codec, tracks active operations, forwards
 * progress events, and exposes cooperative cancellation. It deliberately does
 * not parse any particular format itself.
 *
 *
 * Accepted calls emit Started followed by exactly one Completed or Error event,
 * including cancellation and codec exceptions. Invalid/rejected calls emit no events.
 * Cancellation is checked before codec dispatch, not inside third-party parsers.
 * Overlapping calls share one cancellation batch; the next idle-to-active transition resets it.
 * Observer callbacks run synchronously on the calling thread and may overlap.
 * They must not call shutdown()/waitUntilIdle() on this service or destroy it.
 * Observers must outlive active operations, even after clearObserver() or replacement.
 * Callers synchronize properties, mesh mutation and non-thread-safe codec implementations.
 * Service/codec destruction and plugin unloading require external lifetime coordination.
 *
 *
 * @warning Registered codecs are borrowed. Unregister them and wait for the
 *          service to become idle before unloading their plugin.
 */
class GAFileIOService : public GAFileIOAssembly
{
public:
    GAFileIOService();
    ~GAFileIOService() override;

    bool init() override;
    void shutdown() noexcept override;
    [[nodiscard]] bool isInitialized() const noexcept override;

    [[nodiscard]] const char* serviceName() const noexcept override;
    [[nodiscard]] std::uint32_t serviceVersion() const noexcept override;
    bool setProperty(const char* name, const char* value) override;
    [[nodiscard]] const char* property(const char* name) const noexcept override;

    [[nodiscard]] bool registerCodec(GAFileCodec* codec) override;
    [[nodiscard]] bool unregisterCodec(const char* formatId, const char* backendId = nullptr) noexcept override;
    [[nodiscard]] const GAFileCodec* codec(const char* formatId,
                                           const char* backendId = nullptr) const noexcept override;
    [[nodiscard]] std::size_t codecCount() const noexcept override;

    // An empty query lists registered implementations; filters narrow their advertised capabilities.
    [[nodiscard]] std::vector<GAFileCodecInfo> queryCodecs(const GAFileCodecQuery& query = {}) const override;

    void setObserver(IGAFileObserver& observer) noexcept override;
    void clearObserver() noexcept override;

    void requestCancellation() noexcept override;
    [[nodiscard]] bool cancellationRequested() const noexcept override;
    [[nodiscard]] bool waitUntilIdle(std::uint64_t timeoutMilliseconds) override;

    /// Selects a codec and performs one synchronous import operation.
    [[nodiscard]] GAFileResult read(const GAFileReadRequest& request, GADataStore& store) override;

    /// Selects a codec and performs one synchronous export operation.
    [[nodiscard]] GAFileResult write(const GAFileWriteRequest& request, const GADataStore& store) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
