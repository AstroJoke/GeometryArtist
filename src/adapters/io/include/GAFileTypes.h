#pragma once

#include "GADataTypes.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief Bit flags describing operations and content supported by a file codec.
 *
 */
enum class GAFileCapability : std::uint32_t
{
    None = 0,
    Read = 1U << 0,
    Write = 1U << 1,
    Binary = 1U << 2,
    Scene = 1U << 3,
    Mesh = 1U << 4,
    Streaming = 1U << 5
};

[[nodiscard]] constexpr GAFileCapability operator|(GAFileCapability lhs, GAFileCapability rhs) noexcept
{
    return static_cast<GAFileCapability>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

enum class GAFileStatus : std::uint8_t
{
    Success,
    InvalidArgument,
    FormatNotFound,
    OpenFailed,
    ParseFailed,
    WriteFailed,
    Unsupported,
    Cancelled,
    SelectionRequired
};

/**
 * @brief Static metadata used to discover and select a file codec.
 *
 * All character pointers and the extension array are borrowed. Their owner must
 * keep them valid for at least as long as the registered codec remains active.
 *
 */
struct GAFileFormatDescriptor
{
    const char* id = nullptr;
    const char* displayName = nullptr;
    const char* const* extensions = nullptr;
    std::size_t extensionCount = 0;
    GAFileCapability capabilities = GAFileCapability::None;
    const char* backendId = nullptr;
    const char* backendName = nullptr;
};

// All specified filters must match. An explicit format takes precedence over the path.
// Capabilities describe advertised support, not whether a particular file is valid.
struct GAFileCodecQuery
{
    const char* backendId = nullptr;
    const char* formatId = nullptr;
    const char* filePath = nullptr;
    GAFileCapability required = GAFileCapability::None;
};

// Owns its metadata so callers can retain query results after unregistering a plugin.
struct GAFileCodecInfo
{
    std::string backendId;
    std::string backendName;
    std::string formatId;
    std::string displayName;
    std::vector<std::string> extensions;
    GAFileCapability capabilities = GAFileCapability::None;
};

/// Complete value object describing one import operation.
/// filePath is a UTF-8 path to a file.
struct GAFileReadRequest
{
    const char* filePath = nullptr;
    const char* formatId = nullptr;
    bool triangulate = true;
    bool generateNormals = false;
    // Null permits automatic selection only when exactly one implementation matches.
    const char* backendId = nullptr;
};

/// Complete value object describing one export operation.
/// filePath is the complete UTF-8 destination filename, not a directory.
struct GAFileWriteRequest
{
    const char* filePath = nullptr;
    const char* formatId = nullptr;
    GADataHandle input = GAInvalidDataHandle;
    bool binary = false;
    const char* backendId = nullptr;
};

/**
 * @brief Result returned by a file operation.
 *
 * On a successful read, data identifies the object registered in the supplied
 * store. message is borrowed and must not be retained by the caller.
 *
 */
struct GAFileResult
{
    GAFileStatus status = GAFileStatus::Unsupported;
    GADataHandle data = GAInvalidDataHandle;
    std::uint64_t bytesProcessed = 0;
    const char* message = nullptr;
};

enum class GAFileEventType : std::uint8_t
{
    Started,
    Progress,
    Completed,
    Warning,
    Error
};

/**
 * @brief Transient progress notification delivered synchronously to an observer.
 *
 */
struct GAFileEvent
{
    GAFileEventType type = GAFileEventType::Progress;
    const char* filePath = nullptr;
    double progress = 0.0;
    GAFileStatus status = GAFileStatus::Success;
    const char* message = nullptr;
};
