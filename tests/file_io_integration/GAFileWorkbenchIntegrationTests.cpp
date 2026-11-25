#include "GAArtifactStore.h"
#include "GADataStore.h"
#include "GAFileIOService.h"
#include "GAFilePluginLoader.h"
#include "GAFileUseCases.h"
#include "IGAFileReader.h"
#include "GAMeshDataObject.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <filesystem>
#include <fstream>
#include <iostream>
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

class RejectPublication : public GAArtifactStore
{
public:
    bool throwOnCreate = false;

    GAArtifactId create(GAArtifactType, std::string_view, std::string_view, GADataHandle) override
    {
        if (throwOnCreate)
        {
            throw std::runtime_error("Publication error");
        }
        return GAInvalidArtifactId;
    }
};

class InvalidHandleReader : public IGAFileReader
{
public:
    GAFileResult read(const GAFileReadRequest&, GADataStore&) override
    {
        return {GAFileStatus::Success, GAInvalidDataHandle};
    }
};

std::string utf8(const std::filesystem::path& path)
{
    const auto value = path.u8string();
    return std::string(value.begin(), value.end());
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    try
    {
        check(argc == 2, "Expected mesh plugin path");
        QTemporaryDir temporary;
        check(temporary.isValid(), "Cannot create temporary directory");
        const auto rootUtf8 = temporary.path().toUtf8();
        const std::filesystem::path root(std::u8string(rootUtf8.constData(), rootUtf8.constData() + rootUtf8.size()));
        const auto inputPath = root / "quad.obj";
        std::ofstream fixture(inputPath, std::ios::binary);
        fixture << "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nf 1 2 3 4\n";
        fixture.close();
        check(!fixture.fail(), "Cannot write test fixture");

        // The loader outlives service registrations and their active calls.
        GAFilePluginLoader loader;
        check(loader.load(argv[1]), loader.errorString().c_str());
        GAFileIOService service;
        check(service.init(), "Service init failed");
        for (std::size_t i = 0; i < loader.codecCount(); ++i)
        {
            check(service.registerCodec(loader.codec(i)), "Codec registration failed");
        }
        GADataStore store;
        GAArtifactStore artifacts;
        GAImportUseCase importUseCase(service, store, artifacts);
        const auto inputName = utf8(inputPath);
        GAFileReadRequest request{inputName.c_str(), "mesh.obj"};
        request.backendId = "cgal";
        request.triangulate = false;
        check(importUseCase.execute(request).status == GAFileStatus::Unsupported && store.size() == 0 &&
                  artifacts.size() == 0, "A quad requires explicit triangulation");
        request.triangulate = true;
        request.generateNormals = true;
        const auto artifact = importUseCase.execute(request);
        check(artifact.status == GAFileStatus::Success && artifacts.size() == 1, "Workbench artifact publication");
        check(store.contains(artifacts.dataHandle(artifact.artifact)), "Artifact must reference registered data");
        const auto original = artifacts.dataHandle(artifact.artifact);
        const auto mesh = std::dynamic_pointer_cast<const GAMeshDataObject>(store.access(original));
        check(mesh && mesh->indexCount() == 6 && mesh->view().normalCount == mesh->vertexCount(),
              "The imported quad must have two triangles and vertex normals");
        request.backendId = "missing-backend";
        const auto missing = importUseCase.execute(request);
        check(missing.status == GAFileStatus::FormatNotFound && artifacts.size() == 1 && store.size() == 1,
              "An explicit backend must be honored");

        request.backendId = "cgal";
        RejectPublication rejected;
        GAImportUseCase rejectedImport(service, store, rejected);
        const auto failed = rejectedImport.execute(request);
        check(failed.status == GAFileStatus::ParseFailed && failed.artifact == GAInvalidArtifactId &&
                  store.size() == 1 && store.contains(original) && rejected.size() == 0,
              "A rejected publication must release only the new data");
        rejected.throwOnCreate = true;
        bool threw = false;
        try
        {
            (void)rejectedImport.execute(request);
        }
        catch (const std::runtime_error& error)
        {
            threw = std::string(error.what()) == "Publication error";
        }
        check(threw && store.size() == 1 && store.contains(original) && rejected.size() == 0,
              "A publication exception must release only the new data");

        InvalidHandleReader invalidReader;
        GAImportUseCase invalidImport(invalidReader, store, artifacts);
        const auto invalid = invalidImport.execute(request);
        check(invalid.status == GAFileStatus::ParseFailed && invalid.artifact == GAInvalidArtifactId &&
                  artifacts.size() == 1, "Reader success requires a live data handle");
        service.shutdown();
        loader.unload();
        std::cout << "File/Workbench integration passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
