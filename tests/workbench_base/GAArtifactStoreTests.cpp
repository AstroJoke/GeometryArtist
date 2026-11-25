#include "GAArtifactStore.h"
#include "GADataStore.h"
#include "GAMeshDataObject.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
void check(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void metadata()
{
    GADataStore data;
    const auto first = data.registe(std::make_unique<GAMeshDataObject>());
    const auto replacement = data.registe(std::make_unique<GAMeshDataObject>());
    GAArtifactStore store;
    IGAArtifactRepository& artifacts = store;
    std::string name = "Imported mesh";
    std::string media = "application/x-geometry-artist-data";
    const auto id = artifacts.create(GAArtifactType::Mesh, name, media, first);
    check(id != GAInvalidArtifactId && artifacts.size() == 1, "Artifact registration failed");
    name.assign("changed source");
    media.clear();
    GAArtifactDescriptor original;
    check(artifacts.descriptor(id, original), "Artifact lookup failed");
    check(original.id == id && original.type == GAArtifactType::Mesh && original.version == 1 &&
              original.data == first && original.name == "Imported mesh" &&
              original.mediaType == "application/x-geometry-artist-data",
          "Descriptor borrowed input strings or lost metadata");
    check(artifacts.updateData(id, replacement), "Artifact data update failed");
    GAArtifactDescriptor updated;
    check(artifacts.descriptor(id, updated) && updated.version == 2 && updated.data == replacement,
          "Artifact update did not advance its version");
    check(updated.name == original.name && updated.mediaType == original.mediaType,
          "Updating a handle changed unrelated metadata");
    check(original.version == 1 && original.data == first, "Returned descriptor was not a snapshot");
    check(artifacts.remove(id) && artifacts.size() == 0, "Artifact removal failed");
    check(original.name == "Imported mesh", "Snapshot lost its owned metadata after removal");
    const auto second = artifacts.create(GAArtifactType::Mesh, "Second", "mesh", first);
    artifacts.clear();
    const auto third = artifacts.create(GAArtifactType::Mesh, "Third", "mesh", first);
    check(third != second && third != id && artifacts.dataHandle(second) == GAInvalidDataHandle,
          "Clearing the repository reused a stale artifact ID");
}

void lifetime()
{
    GADataStore data;
    const auto handle = data.registe(std::make_unique<GAMeshDataObject>());
    GAArtifactStore artifacts;
    auto id = artifacts.create(GAArtifactType::Mesh, "mesh", "mesh", handle);
    check(artifacts.remove(id) && data.contains(handle), "Artifact removal released data it did not own");
    id = artifacts.create(GAArtifactType::Mesh, "mesh", "mesh", handle);
    check(data.release(handle), "Could not release data");
    GAArtifactDescriptor descriptor;
    check(artifacts.descriptor(id, descriptor) && descriptor.data == handle && !data.access(handle),
          "Repository unexpectedly owned data or hid a stale association");
    const auto retained = data.registe(std::make_unique<GAMeshDataObject>());
    check(artifacts.create(GAArtifactType::Mesh, "retained", "mesh", retained) != GAInvalidArtifactId,
          "Second artifact registration failed");
    artifacts.clear();
    check(data.contains(retained), "Clearing metadata released data");
    {
        GAArtifactStore temporary;
        check(temporary.create(GAArtifactType::Mesh, "temporary", "mesh", retained) != GAInvalidArtifactId,
              "Temporary artifact registration failed");
    }
    check(data.contains(retained), "Repository destruction released data");
}

void invalid()
{
    GADataStore data;
    const auto handle = data.registe(std::make_unique<GAMeshDataObject>());
    GAArtifactStore artifacts;
    check(artifacts.create(GAArtifactType::Unknown, "", "", handle) == GAInvalidArtifactId,
          "Unknown artifact type accepted");
    check(artifacts.create(GAArtifactType::Mesh, "", "", GAInvalidDataHandle) == GAInvalidArtifactId &&
              artifacts.size() == 0,
          "Invalid data handle created metadata");
    const auto id = artifacts.create(GAArtifactType::Mesh, "mesh", "mesh", handle);
    check(!artifacts.updateData(id, GAInvalidDataHandle) && !artifacts.updateData(GAInvalidArtifactId, handle),
          "Invalid update accepted");
    GAArtifactDescriptor snapshot;
    check(artifacts.descriptor(id, snapshot) && snapshot.version == 1 && snapshot.data == handle,
          "Invalid update changed metadata");
    check(!artifacts.descriptor(GAInvalidArtifactId, snapshot) && snapshot.id == id && snapshot.name == "mesh",
          "Missing lookup changed the caller's output");
    check(!artifacts.remove(GAInvalidArtifactId) && artifacts.size() == 1,
          "Missing removal changed the repository");
    check(artifacts.remove(id) && !artifacts.remove(id) && !artifacts.updateData(id, handle) &&
              artifacts.dataHandle(id) == GAInvalidDataHandle,
          "Removed artifact remained accessible");
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        check(argc == 2, "Expected metadata, lifetime or invalid");
        const std::string_view testCase(argv[1]);
        if (testCase == "metadata")
        {
            metadata();
        }
        else if (testCase == "lifetime")
        {
            lifetime();
        }
        else if (testCase == "invalid")
        {
            invalid();
        }
        else
        {
            throw std::runtime_error("Unknown test case");
        }
        std::cout << "Artifact " << testCase << " checks passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
