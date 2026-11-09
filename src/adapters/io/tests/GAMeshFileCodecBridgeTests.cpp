#include "GAMeshFileCodec.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
void check(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

class FakeBackend : public IGAMeshFileBackend
{
public:
    int decodeCalls = 0;
    int encodeCalls = 0;
    bool failRead = false;
    bool throwRead = false;
    bool nullRead = false;
    bool invalidRead = false;
    bool failWrite = false;
    bool throwWrite = false;
    GAMeshDecodeOptions lastOptions;

    GAMeshDecodeResult decode(std::istream&, const GAMeshDecodeOptions& options) override
    {
        ++decodeCalls;
        lastOptions = options;
        if (throwRead)
        {
            throw std::runtime_error("Fake parser failure");
        }
        if (failRead)
        {
            return {GAFileStatus::ParseFailed, nullptr, "Fake decode failed"};
        }
        if (nullRead)
        {
            return {GAFileStatus::Success, nullptr, "Broken backend"};
        }
        auto object = std::make_unique<GAMeshDataObject>();
        const double x = invalidRead ? std::numeric_limits<double>::quiet_NaN() : 0.0;
        const double positions[] = {x, 0, 0, 1, 0, 0, 0, 1, 0};
        const std::uint32_t indices[] = {0, 1, 2};
        check(object->setData({positions, 3, indices, 3}), "Fake setup");
        return {GAFileStatus::Success, std::move(object), "Fake decode succeeded"};
    }

    GAMeshEncodeResult encode(std::ostream& stream, const GAMeshDataView& view, bool) override
    {
        ++encodeCalls;
        check(view.indexCount == 3, "Backend did not receive neutral data");
        stream << "new data";
        if (throwWrite)
        {
            throw std::runtime_error("Fake encoder failure");
        }
        return {failWrite ? GAFileStatus::WriteFailed : GAFileStatus::Success, "Fake encode result"};
    }
};

class TestCodec : public GAMeshFileCodec
{
public:
    explicit TestCodec(std::unique_ptr<IGAMeshFileBackend> backend)
        : GAMeshFileCodec(std::move(backend))
    {
    }

    const GAFileFormatDescriptor& descriptor() const noexcept override
    {
        static constexpr const char* extensions[] = {"fake"};
        static constexpr GAFileFormatDescriptor info{"test.fake",
                                                     "Injected fake backend",
                                                     extensions,
                                                     1,
                                                     GAFileCapability::Read | GAFileCapability::Write |
                                                         GAFileCapability::Mesh,
                                                     "test.fake",
                                                     "Fake backend"};
        return info;
    }
};

std::string readText(const std::string& path)
{
    std::ifstream input(std::filesystem::path(path), std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    try
    {
        QTemporaryDir temporary;
        check(temporary.isValid(), "Temporary directory");
        const std::string path = (temporary.path() + "/data.fake").toStdString();
        {
            std::ofstream file(path);
            file << "old data";
        }
        auto backend = std::make_unique<FakeBackend>();
        auto* fake = backend.get();
        TestCodec codec(std::move(backend));
        GADataStore store;
        check(codec.acceptsExtension(".FAKE"), "Extension metadata not used");
        const auto good = codec.read({path.c_str(), nullptr, false, true}, store);
        check(good.status == GAFileStatus::Success && store.size() == 1, "Bridge did not publish neutral object");
        check(!fake->lastOptions.triangulate && fake->lastOptions.generateNormals, "Options not forwarded");

        fake->failRead = true;
        check(codec.read({path.c_str()}, store).status == GAFileStatus::ParseFailed, "Backend error lost");
        fake->failRead = false;
        fake->throwRead = true;
        check(codec.read({path.c_str()}, store).status == GAFileStatus::ParseFailed, "Exception escaped bridge");
        fake->throwRead = false;
        fake->nullRead = true;
        check(codec.read({path.c_str()}, store).status == GAFileStatus::ParseFailed, "Null object published");
        fake->nullRead = false;
        fake->invalidRead = true;
        check(codec.read({path.c_str()}, store).status == GAFileStatus::ParseFailed, "Invalid neutral data published");
        fake->invalidRead = false;
        check(store.size() == 1, "Failed reads changed store");

        fake->failWrite = true;
        check(codec.write({path.c_str(), nullptr, good.data}, store).status == GAFileStatus::WriteFailed,
              "Backend write failure lost");
        check(readText(path) == "old data", "Failed encode changed destination");
        fake->failWrite = false;
        fake->throwWrite = true;
        check(codec.write({path.c_str(), nullptr, good.data}, store).status == GAFileStatus::WriteFailed,
              "Write exception escaped bridge");
        check(readText(path) == "old data", "Throwing encode changed destination");
        fake->throwWrite = false;
        const auto calls = fake->encodeCalls;
        check(codec.write({path.c_str(), nullptr, good.data, true}, store).status == GAFileStatus::Unsupported,
              "Binary capability not enforced");
        check(fake->encodeCalls == calls, "Unsupported request reached backend");
        check(codec.write({path.c_str(), nullptr, good.data}, store).status == GAFileStatus::Success,
              "Successful encode did not commit");
        check(readText(path) == "new data", "Committed bytes differ");
        std::cout << "Backend-independent bridge tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
