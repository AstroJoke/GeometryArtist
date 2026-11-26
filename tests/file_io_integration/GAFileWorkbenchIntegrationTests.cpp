#include "GAArtifactStore.h"
#include "GADataStore.h"
#include "GAFileIOService.h"
#include "GAFilePluginLoader.h"
#include "GAFileUseCases.h"
#include "GAWorkbenchController.h"
#include "IGAWorkbenchView.h"
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

class DocumentView : public IGAWorkbenchView
{
public:
    void setActiveArtifact(GAArtifactId id) override { active = id; }
    void showStatus(const char* text) override { status = text; }
    void showError(const char* text) override { error = text; }

    GAArtifactId active = GAInvalidArtifactId;
    std::string status, error;
};

void checkDocumentCommands(GAFileIOService& files, const std::string& input, const std::filesystem::path& root)
{
    GADataStore data;
    GAArtifactStore artifacts;
    GAImportUseCase importer(files, data, artifacts);
    GAExportUseCase exporter(files, data, artifacts);
    DocumentView view;
    GAWorkbenchController controller(importer, exporter, view);
    check(controller.openDocument(input.c_str()) && data.contains(artifacts.dataHandle(view.active)) &&
              !view.status.empty(), "Opening a mesh selects the published artifact");
    const auto selected = view.active;
    GAFileReadRequest request{input.c_str(), "mesh.obj"};
    request.backendId = "missing-backend";
    check(!controller.openDocument(request) && !view.error.empty() && view.active == selected && artifacts.size() == 1,
          "A failed open reports the error and keeps the selection");

    const auto output = root / "document.output";
    const auto path = utf8(output);
    view.error.clear();
    check(!controller.saveDocument(selected, path.c_str(), "mesh.ply", true, "missing-backend") &&
              !view.error.empty() && !std::filesystem::exists(output), "Saving honors backend selection");
    view.status.clear();
    check(controller.saveDocument(selected, path.c_str(), "mesh.ply", true, "cgal") &&
              std::filesystem::file_size(output) > 0 && !view.status.empty(), "Saving reports a successful export");
}

void checkExport(GAFileIOService& service, GADataStore& store, GAArtifactStore& artifacts,
                 GAArtifactId artifact, const std::filesystem::path& root)
{
    GAExportUseCase exporter(service, store, artifacts);
    const auto off = utf8(root / "mesh.off");
    check(exporter.execute(artifact, off.c_str()).status == GAFileStatus::Success,
          "Export by extension with default options");

    const auto output = root / "mesh.output";
    const auto path = utf8(output);
    check(exporter.execute(GAInvalidArtifactId, path.c_str()).status == GAFileStatus::InvalidArgument,
          "Unknown artifact");
    check(exporter.execute(artifact, "").status == GAFileStatus::InvalidArgument, "Empty output path");
    check(exporter.execute(artifact, path.c_str(), "mesh.ply", true, "missing-backend").status ==
              GAFileStatus::FormatNotFound, "Explicit backend must be honored");
    check(exporter.execute(artifact, path.c_str(), "mesh.obj", true, "cgal").status ==
              GAFileStatus::Unsupported, "OBJ does not support binary output");
    check(!std::filesystem::exists(output), "Rejected requests must not create the destination");

    const auto written = exporter.execute(artifact, path.c_str(), "mesh.ply", true, "cgal");
    check(written.status == GAFileStatus::Success && written.bytesProcessed == std::filesystem::file_size(output),
          "Explicit PLY format with a custom extension");
    std::ifstream file(output, std::ios::binary);
    std::string magic;
    std::string format;
    std::getline(file, magic);
    std::getline(file, format);
    check(magic.starts_with("ply") && format.starts_with("format binary_"), "Expected a binary PLY header");

    const auto missingDir = utf8(root / "missing" / "mesh.off");
    check(exporter.execute(artifact, missingDir.c_str()).status == GAFileStatus::OpenFailed,
          "File open failure must reach the caller");

    check(store.release(artifacts.dataHandle(artifact)), "Release data while retaining its artifact");
    const auto stale = utf8(root / "stale.off");
    check(exporter.execute(artifact, stale.c_str()).status == GAFileStatus::InvalidArgument &&
              !std::filesystem::exists(root / "stale.off"), "Reject artifacts whose data has been released");
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
        checkDocumentCommands(service, inputName, root);
        checkExport(service, store, artifacts, artifact.artifact, root);
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
