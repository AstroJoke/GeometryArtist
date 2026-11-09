#pragma once

#include <cstdint>

/**
 * @brief Cooperative cancellation and quiescence control for file operations.
 *
 * Cancellation is a request, not forced thread termination. "Idle" means that
 * no read or write call is currently executing through the service.
 *
 */
class GAFileExecutionPolicy
{
public:
    virtual ~GAFileExecutionPolicy() = default;

    virtual void requestCancellation() noexcept = 0;
    [[nodiscard]] virtual bool cancellationRequested() const noexcept = 0;
    /// A timeout of zero means wait without a deadline.
    [[nodiscard]] virtual bool waitUntilIdle(std::uint64_t timeoutMilliseconds) = 0;
};
