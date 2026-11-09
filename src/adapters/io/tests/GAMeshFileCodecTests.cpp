#include "GAFileIOService.h"
#include "GAFilePluginLoader.h"
#include "GAMeshDataObject.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <algorithm>
#include <array>
#include <cmath>
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

std::string utf8(const std::filesystem::path& path)
{
    const auto value = path.u8string();
    return std::string(value.begin(), value.end());
}

void fixture(const std::filesystem::path& path, const std::string& content)
{
    std::ofstream output(path, std::ios::binary);
    output << content;
    output.close();
    check(!output.fail(), "Cannot write test fixture");
}

std::string contents(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
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

        // Loader outlives service registrations; returned GA meshes own neutral buffers.
        GAFilePluginLoader loader;
        check(loader.load(argv[1]), loader.errorString().c_str());
        GAFileIOService service;
        check(service.init(), "Service init failed");
        for (std::size_t i = 0; i < loader.codecCount(); ++i)
        {
            check(service.registerCodec(loader.codec(i)), "Codec registration failed");
        }
        check(service.codecCount() == 4, "Expected four CGAL codec descriptors");
        const auto catalogue = service.queryCodecs({.backendId = "cgal"});
        check(catalogue.size() == 4 && catalogue.front().backendName == "CGAL", "CGAL backend identity");
        check(service.queryCodecs({.backendId = "cgal", .required = GAFileCapability::Write | GAFileCapability::Binary})
                      .size() == 2,
              "Only PLY and STL advertise binary export");
        check(service.queryCodecs({.backendId = "cgal", .formatId = "mesh.obj", .required = GAFileCapability::Binary})
                  .empty(),
              "OBJ must not advertise binary export");
        check(service.queryCodecs({.backendId = "cgal", .filePath = "test.ObJ", .required = GAFileCapability::Read})
                      .size() == 1,
              "CGAL extension query");
        check(service.codec("mesh.obj")->acceptsExtension(".ObJ"), "Case-insensitive extension lookup");
        GADataStore store;

        const auto objPath = root / std::filesystem::path(u8"triangle-测试.OBJ");
        fixture(objPath, "v 0 0 0\nv 1.123456789012345 0 0\nv 0 1 0\nf 1 2 3\n");
        const auto objName = utf8(objPath);
        const auto imported = service.read({objName.c_str(), nullptr, true, true, "cgal"}, store);
        check(imported.status == GAFileStatus::Success, "OBJ import failed");
        check(service.read({.filePath = objName.c_str(), .backendId = "missing"}, store).status ==
                  GAFileStatus::FormatNotFound,
              "Unknown engine fell back to CGAL");
        check(imported.bytesProcessed == std::filesystem::file_size(objPath), "Input byte count");
        const auto original = std::dynamic_pointer_cast<const GAMeshDataObject>(store.access(imported.data));
        check(original && original->vertexCount() == 3 && original->indexCount() == 3, "OBJ geometry");
        check(original->view().normalCount == 3, "Requested normal generation");

        for (const char* extension : {"obj", "off", "ply", "stl"})
        {
            for (bool binary : {false, true})
            {
                if (binary && std::string(extension) != "ply" && std::string(extension) != "stl")
                {
                    continue;
                }
                const auto path = root / (std::string(binary ? "binary." : "ascii.") + extension);
                const auto name = utf8(path);
                const auto written = service.write({name.c_str(), nullptr, imported.data, binary}, store);
                check(written.status == GAFileStatus::Success, "Round-trip export failed");
                check(written.bytesProcessed == std::filesystem::file_size(path), "Output byte count");
                const auto read = service.read({name.c_str()}, store);
                check(read.status == GAFileStatus::Success, "Round-trip import failed");
                const auto mesh = std::dynamic_pointer_cast<const GAMeshDataObject>(store.access(read.data));
                check(mesh && mesh->vertexCount() == 3 && mesh->indexCount() == 3, "Round-trip topology");
                double maxX = 0;
                for (std::size_t i = 0; i < mesh->vertexCount(); ++i)
                {
                    maxX = std::max(maxX, mesh->view().positions[i * 3]);
                }
                const double tolerance = binary && std::string(extension) == "stl" ? 1e-6 : 1e-14;
                check(std::abs(maxX - 1.123456789012345) < tolerance, "Round-trip coordinate precision");
            }
        }

        const auto polygonPath = root / "polygon.obj";
        fixture(polygonPath, "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nf 1 2 3 4\n");
        const auto polygonName = utf8(polygonPath);
        const auto countBefore = store.size();
        check(service.read({polygonName.c_str(), nullptr, false}, store).status == GAFileStatus::Unsupported,
              "Polygon without triangulation must be rejected");
        check(store.size() == countBefore, "Failed read must not publish a handle");
        const auto triangulated = service.read({polygonName.c_str()}, store);
        check(triangulated.status == GAFileStatus::Success, "Polygon triangulation failed");
        check(std::dynamic_pointer_cast<const GAMeshDataObject>(store.access(triangulated.data))->indexCount() == 6,
              "Quad should produce two triangles");

        const auto badPath = root / "bad.obj";
        fixture(badPath, "not a mesh\n");
        const auto badName = utf8(badPath);
        const auto countAfter = store.size();
        check(service.read({badName.c_str()}, store).status == GAFileStatus::ParseFailed, "Malformed input status");
        fixture(badPath, "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 999\n");
        check(service.read({badName.c_str()}, store).status == GAFileStatus::ParseFailed, "Bad face indices");
        fixture(badPath, "");
        check(service.read({badName.c_str()}, store).status == GAFileStatus::ParseFailed, "Empty input status");
        const auto missingName = utf8(root / "missing.obj");
        check(service.read({missingName.c_str()}, store).status == GAFileStatus::OpenFailed, "Missing file status");
        check(store.size() == countAfter, "Failed imports changed store");
        check(service.read({""}, store).status == GAFileStatus::FormatNotFound, "Empty service path");
        const auto explicitRead = service.read({objName.c_str(), "unknown.format"}, store);
        check(explicitRead.status == GAFileStatus::FormatNotFound, "Explicit format must not fall back");

        const auto protectedPath = root / "protected.obj";
        const auto protectedName = utf8(protectedPath);
        fixture(protectedPath, "keep me");
        check(service.write({protectedName.c_str(), nullptr, imported.data, true}, store).status ==
                  GAFileStatus::Unsupported,
              "OBJ binary must be unsupported");
        check(contents(protectedPath) == "keep me", "Unsupported write damaged destination");
        check(service.write({protectedName.c_str(), nullptr, 999999}, store).status == GAFileStatus::InvalidArgument,
              "Invalid handle status");
        check(contents(protectedPath) == "keep me", "Invalid handle damaged destination");

        auto malformed = std::make_unique<GAMeshDataObject>();
        const double positions[] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
        const std::uint32_t partialTriangle[] = {0, 1};
        check(malformed->setData({positions, 3, partialTriangle, 2}), "Fixture mesh setup");
        const auto malformedHandle = store.registe(std::move(malformed));
        check(service.write({protectedName.c_str(), nullptr, malformedHandle}, store).status ==
                  GAFileStatus::InvalidArgument,
              "Incomplete triangle must not export");
        check(contents(protectedPath) == "keep me", "Invalid mesh damaged destination");

        // Format-specific limits must not leak into the shared CGAL conversion.
        auto wideMesh = std::make_unique<GAMeshDataObject>();
        const double widePositions[] = {0, 0, 0, 1.0e40, 0, 0, 0, 1.0e40, 0};
        const std::uint32_t triangle[] = {0, 1, 2};
        check(wideMesh->setData({widePositions, 3, triangle, 3}), "Wide coordinate fixture");
        const auto wideHandle = store.registe(std::move(wideMesh));
        const auto wideOff = utf8(root / "wide.off");
        check(service.write({wideOff.c_str(), nullptr, wideHandle}, store).status == GAFileStatus::Success,
              "STL float limit leaked into OFF conversion");
        const auto wideStl = root / "wide.stl";
        const auto wideStlName = utf8(wideStl);
        fixture(wideStl, "keep original");
        check(service.write({wideStlName.c_str(), nullptr, wideHandle, true}, store).status ==
                  GAFileStatus::InvalidArgument,
              "Binary STL range validation lost during extraction");
        check(contents(wideStl) == "keep original", "Out-of-range STL export damaged destination");

        const std::array<std::array<double, 9>, 3> collapsingTriangles{{
            {1e9, 1e9, 0, 1e9 + 1, 1e9, 0, 1e9, 1e9 + 1, 0},
            {0, 0, 0, 1, 1, 0, 2, 2 + 1e-8, 0},
            {0, 0, 0, 1e-50, 0, 0, 0, 1e-50, 0}
        }};
        const auto precisionPath = root / "precision.stl";
        const auto precisionName = utf8(precisionPath);
        const auto asciiName = utf8(root / "precision-ascii.stl");
        for (const auto& points : collapsingTriangles)
        {
            auto preciseMesh = std::make_unique<GAMeshDataObject>();
            check(preciseMesh->setData({points.data(), 3, triangle, 3}), "Precision fixture setup");
            const auto preciseHandle = store.registe(std::move(preciseMesh));
            fixture(precisionPath, "keep precise original");
            check(service.write({precisionName.c_str(), nullptr, preciseHandle, true}, store).status ==
                      GAFileStatus::InvalidArgument,
                  "Binary STL accepted a triangle collapsed by float rounding");
            check(contents(precisionPath) == "keep precise original", "Precision failure replaced destination");
            check(service.write({asciiName.c_str(), nullptr, preciseHandle, false}, store).status ==
                      GAFileStatus::Success && service.read({asciiName.c_str()}, store).status == GAFileStatus::Success,
                  "Binary STL precision restriction leaked into ASCII STL");
            const auto retained = std::dynamic_pointer_cast<const GAMeshDataObject>(store.access(preciseHandle));
            check(std::equal(points.begin(), points.end(), retained->view().positions),
                  "Export changed the source mesh coordinates");
        }
        auto representable = std::make_unique<GAMeshDataObject>();
        const double representablePoints[]{1e9, 1e9, 0, 1e9 + 128, 1e9, 0, 1e9, 1e9 + 128, 0};
        check(representable->setData({representablePoints, 3, triangle, 3}), "Representable fixture setup");
        const auto representableHandle = store.registe(std::move(representable));
        check(service.write({precisionName.c_str(), nullptr, representableHandle, true}, store).status ==
                  GAFileStatus::Success && service.read({precisionName.c_str()}, store).status == GAFileStatus::Success,
              "Representable large-coordinate binary STL was rejected");

        // Data must remain usable after all codec objects and their library are gone.
        service.shutdown();
        loader.unload();
        check(catalogue.size() == 4 && catalogue.front().backendName == "CGAL" && !catalogue.front().extensions.empty(),
              "Query results must survive plugin unload");
        check(original->vertexCount() == 3 && store.contains(imported.data), "Plugin lifetime leaked into data");
        std::cout << "Mesh codec integration tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
