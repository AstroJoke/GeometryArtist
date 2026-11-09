#include "GAMeshFileCodec.h"

#include <QSaveFile>
#include <QString>

#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <limits>
#include <locale>
#include <ostream>
#include <stdexcept>
#include <streambuf>
#include <string_view>
#include <utility>

namespace
{
GAFileResult failure(GAFileStatus status, const char* message)
{
    return {status, GAInvalidDataHandle, 0, message};
}

bool equalExtension(const char* lhs, const char* rhs) noexcept
{
    if (lhs == nullptr || rhs == nullptr)
    {
        return false;
    }
    if (*lhs == '.')
    {
        ++lhs;
    }
    const auto lower = [](unsigned char c)
    {
        return c >= 'A' && c <= 'Z' ? static_cast<unsigned char>(c + ('a' - 'A')) : c;
    };
    while (*lhs != '\0' && *rhs != '\0')
    {
        if (lower(static_cast<unsigned char>(*lhs++)) != lower(static_cast<unsigned char>(*rhs++)))
        {
            return false;
        }
    }
    return *lhs == *rhs;
}


bool supports(GAFileCapability capabilities, GAFileCapability required) noexcept
{
    return (static_cast<std::uint32_t>(capabilities) & static_cast<std::uint32_t>(required)) != 0;
}

// Storage invariants only. Manifoldness and solver suitability are not file-service policy.
bool validView(const GAMeshDataView& view)
{
    if (view.vertexCount == 0 || view.indexCount == 0 || view.indexCount % 3 != 0 ||
        view.positions == nullptr || view.indices == nullptr ||
        view.vertexCount > std::numeric_limits<std::size_t>::max() / 3 ||
        view.vertexCount > std::numeric_limits<std::uint32_t>::max() ||
        (view.normalCount != 0 && (view.normalCount != view.vertexCount || view.normals == nullptr)) ||
        (view.textureCoordinateCount != 0 &&
         (view.textureCoordinateCount != view.vertexCount || view.textureCoordinates == nullptr)))
    {
        return false;
    }
    for (std::size_t i = 0; i < view.vertexCount * 3; ++i)
    {
        if (!std::isfinite(view.positions[i]))
        {
            return false;
        }
    }
    for (std::size_t i = 0; i < view.indexCount; ++i)
    {
        if (view.indices[i] >= view.vertexCount)
        {
            return false;
        }
    }
    for (std::size_t i = 0; i < view.normalCount * 3; ++i)
    {
        if (!std::isfinite(view.normals[i]))
        {
            return false;
        }
    }
    for (std::size_t i = 0; i < view.textureCoordinateCount * 2; ++i)
    {
        if (!std::isfinite(view.textureCoordinates[i]))
        {
            return false;
        }
    }
    return true;
}

// Stream into a same-directory temporary file; commit replaces the destination
// only after serialization succeeds. Never truncate the destination on failure.
class SaveBuffer : public std::streambuf
{
public:
    explicit SaveBuffer(QSaveFile& file)
        : file_(file)
    {
    }

protected:
    std::streamsize xsputn(const char* data, std::streamsize size) override
    {
        const auto written = file_.write(data, static_cast<qint64>(size));
        return written < 0 ? 0 : static_cast<std::streamsize>(written);
    }

    int_type overflow(int_type value) override
    {
        if (traits_type::eq_int_type(value, traits_type::eof()))
        {
            return traits_type::not_eof(value);
        }
        const char c = traits_type::to_char_type(value);
        return file_.write(&c, 1) == 1 ? value : traits_type::eof();
    }

private:
    QSaveFile& file_;
};


} // namespace

GAMeshFileCodec::GAMeshFileCodec(std::unique_ptr<IGAMeshFileBackend> backend)
    : backend_(std::move(backend))
{
    if (!backend_)
    {
        throw std::invalid_argument("A mesh file backend is required");
    }
}

GAMeshFileCodec::~GAMeshFileCodec() = default;

bool GAMeshFileCodec::acceptsExtension(const char* extension) const noexcept
{
    const auto& format = descriptor();
    for (std::size_t i = 0; i < format.extensionCount; ++i)
    {
        if (equalExtension(extension, format.extensions[i]))
        {
            return true;
        }
    }
    return false;
}

GAFileResult GAMeshFileCodec::read(const GAFileReadRequest& request, GADataStore& store)
{
    if (request.filePath == nullptr || *request.filePath == '\0')
    {
        return failure(GAFileStatus::InvalidArgument, "A UTF-8 input file path is required");
    }
    if (!supports(descriptor().capabilities, GAFileCapability::Read))
    {
        return failure(GAFileStatus::Unsupported, "This codec does not support import");
    }
    try
    {
        const std::string_view utf8Path(request.filePath);
        const std::filesystem::path path(std::u8string(utf8Path.begin(), utf8Path.end()));
        std::ifstream input(path, std::ios::binary);
        input.imbue(std::locale::classic());
        if (!input)
        {
            return failure(GAFileStatus::OpenFailed, "Cannot open input file");
        }
        auto decoded = backend_->decode(input, {request.triangulate, request.generateNormals});
        // Nothing enters the Store until decoding and neutral-buffer validation both succeed.
        if (decoded.status != GAFileStatus::Success)
        {
            return failure(decoded.status, decoded.message);
        }
        if (input.bad() || !decoded.object || !validView(decoded.object->view()))
        {
            return failure(GAFileStatus::ParseFailed, "Backend did not return a complete neutral triangle mesh");
        }
        std::error_code error;
        const auto bytes = std::filesystem::file_size(path, error);
        // Publish only after decoding and GA-level validation succeed.
        const auto handle = store.registe(std::move(decoded.object));
        return {GAFileStatus::Success, handle, error ? 0 : bytes, decoded.message};
    }
    catch (const std::exception&)
    {
        return failure(GAFileStatus::ParseFailed, "Backend decoding or data publication failed");
    }
}

GAFileResult GAMeshFileCodec::write(const GAFileWriteRequest& request, const GADataStore& store)
{
    if (request.filePath == nullptr || *request.filePath == '\0' || request.input == GAInvalidDataHandle)
    {
        return failure(GAFileStatus::InvalidArgument, "A UTF-8 output path and valid data handle are required");
    }
    const auto capabilities = descriptor().capabilities;
    if (!supports(capabilities, GAFileCapability::Write) ||
        (request.binary && !supports(capabilities, GAFileCapability::Binary)))
    {
        return failure(GAFileStatus::Unsupported, "This codec does not support the requested export mode");
    }
    try
    {
        const auto object = std::dynamic_pointer_cast<const GAMeshDataObject>(store.access(request.input));
        // A shared pointer protects lifetime, not concurrent edits to the borrowed buffers.
        if (!object || !validView(object->view()))
        {
            return failure(GAFileStatus::InvalidArgument, "Handle does not identify a valid neutral triangle mesh");
        }
        QSaveFile file(QString::fromUtf8(request.filePath));
        // Never fall back to truncating the destination if temporary-file creation fails.
        // Uncommitted output is discarded on failure/exception.
        file.setDirectWriteFallback(false);
        if (!file.open(QIODevice::WriteOnly))
        {
            return failure(GAFileStatus::OpenFailed, "Cannot create output temporary file");
        }
        SaveBuffer buffer(file);
        std::ostream output(&buffer);
        output.imbue(std::locale::classic());
        const auto encoded = backend_->encode(output, object->view(), request.binary);
        output.flush();
        if (encoded.status != GAFileStatus::Success || !output)
        {
            file.cancelWriting();
            return failure(encoded.status == GAFileStatus::Success ? GAFileStatus::WriteFailed : encoded.status,
                           encoded.message);
        }
        const auto bytes = static_cast<std::uint64_t>(file.pos());
        if (!file.commit())
        {
            return failure(GAFileStatus::WriteFailed, "Cannot commit output file");
        }
        return {GAFileStatus::Success, request.input, bytes, encoded.message};
    }
    catch (const std::exception&)
    {
        return failure(GAFileStatus::WriteFailed, "Backend encoding or file commit failed");
    }
}
