#include "GAComponentRuntime.h"
#include "GADataConverterRegistry.h"
#include "GAMeshDataObject.h"
#include "IGAAssembly.h"

#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

// Public clients retain leases; removing registry entries must not dangle them.
void dataLifetime()
{
    GADataStore store;
    require(store.registe(nullptr) == GAInvalidDataHandle, "Null registration accepted");
    const auto handle = store.registe(std::make_unique<GAMeshDataObject>("old"));
    auto previous = store.access(handle);
    std::weak_ptr<const GADataObject> weak = previous;
    require(previous && previous->id() == handle, "Invalid data identity");
    require(!store.replace(GAInvalidDataHandle, std::make_unique<GAMeshDataObject>()), "Invalid replacement accepted");
    require(!store.replace(handle, nullptr), "Null replacement accepted");
    require(store.replace(handle, std::make_unique<GAMeshDataObject>("new")), "Replacement failed");
    auto current = store.access(handle);
    require(previous->name() == "old" && current->name() == "new", "Replacement changed an existing lease");
    require(current->id() == handle && store.size() == 1, "Replacement lost identity");
    require(store.release(handle) && !store.release(handle), "Release semantics incorrect");
    require(!store.access(handle) && current->name() == "new", "Released lease invalidated");
    require(!weak.expired(), "Old revision was destroyed with a live lease");
    previous.reset();
    require(weak.expired(), "Old revision leaked after lease release");
    const auto next = store.registe(std::make_unique<GAMeshDataObject>("next"));
    require(next != handle, "Stale handle reused");
    auto retained = store.access(next);
    store.clear();
    require(store.size() == 0 && retained->name() == "next", "Clear invalidated retained data");
}

void meshOwnership()
{
    constexpr std::size_t VertexCount = 3;
    std::array<double, VertexCount * 3> positions{0, 0, 0, 1, 0, 0, 0, 1, 0};
    std::array<std::uint32_t, 3> indices{0, 1, 2};
    GAMeshDataObject mesh;
    require(mesh.setData({positions.data(), VertexCount, indices.data(), indices.size()}), "Valid triangle rejected");
    positions[0] = 42;
    indices[0] = 2;
    require(mesh.view().positions[0] == 0 && mesh.view().indices[0] == 0, "Mesh borrowed client storage");
    const auto revision = mesh.revision();
    indices[0] = static_cast<std::uint32_t>(VertexCount);
    require(!mesh.setData({positions.data(), VertexCount, indices.data(), indices.size()}),
            "Out-of-range index accepted");
    require(mesh.revision() == revision && mesh.view().positions[0] == 0, "Rejected update modified mesh");
    require(!mesh.setData({nullptr, VertexCount}), "Missing vertex buffer accepted");
    mesh.clear();
    require(mesh.empty() && mesh.indexCount() == 0 && mesh.revision() > revision, "Mesh clear failed");
}

struct Counters
{
    int created = 0;
    int destroyed = 0;
};
class TestFactory : public IGAComponentFactory
{
public:
    TestFactory(std::shared_ptr<Counters> counters, std::string key = "test.component")
        : counters_(std::move(counters))
        , key_(std::move(key))
    {
    }
    GAComponentDescriptor descriptor() const noexcept override
    {
        return {0, key_, "Test component", 1};
    }
    IGAObject* create() override
    {
        ++counters_->created;
        return new IGAObject;
    }
    void destroy(IGAObject* object) noexcept override
    {
        ++counters_->destroyed;
        delete object;
    }

private:
    std::shared_ptr<Counters> counters_;
    std::string key_;
};

void componentLifetime()
{
    GAComponentRegistry registry;
    auto counters = std::make_shared<Counters>();
    auto factory = std::make_shared<TestFactory>(counters);
    std::weak_ptr<IGAComponentFactory> weak = factory;
    require(!registry.registerFactory(nullptr), "Null factory accepted");
    require(!registry.registerFactory(std::make_shared<TestFactory>(counters, "")), "Empty type accepted");
    require(registry.registerFactory(factory), "Factory registration failed");
    require(!registry.registerFactory(std::make_shared<TestFactory>(counters)), "Duplicate type accepted");
    factory.reset();
    require(!weak.expired(), "Registry did not retain its factory");
    auto lease = registry.find("test.component");
    require(lease != nullptr && registry.size() == 1, "Factory lookup failed");
    // This validates factory-mediated destruction, not dynamic-library unloading.
    {
        auto destroy = [lease](IGAObject* object)
        {
            lease->destroy(object);
        };
        std::unique_ptr<IGAObject, decltype(destroy)> object(lease->create(), destroy);
        require(registry.unregisterFactory("test.component"), "Factory unregister failed");
        require(!registry.find("test.component") && registry.size() == 0, "Unregistered factory discoverable");
        require(!registry.unregisterFactory("test.component"), "Missing factory removed twice");
        require(counters->created == 1 && counters->destroyed == 0, "Registry unexpectedly owns component");
    }
    require(counters->destroyed == 1, "Component was not destroyed by factory");
    lease.reset();
    require(weak.expired(), "Factory leaked");
}

class TestConverter : public IGADataConverter
{
public:
    const char* converterId() const noexcept override
    {
        return "test.mesh";
    }
    GADataObjectType sourceType() const noexcept override
    {
        return GADataObjectType::TriangleMesh;
    }
    GADataObjectType targetType() const noexcept override
    {
        return GADataObjectType::TriangleMesh;
    }
    bool canConvert(const GADataObject& source) const noexcept override
    {
        return source.type() == sourceType();
    }
    GAConversionResult convert(const GADataObject&, const GAConversionOptions&, GADataStore&) override
    {
        return {};
    }
};

void converterLifetime()
{
    GADataConverterRegistry registry;
    auto converter = std::make_shared<TestConverter>();
    std::weak_ptr<IGADataConverter> weak = converter;
    require(!registry.registerConverter(nullptr), "Null converter accepted");
    require(registry.registerConverter(converter), "Converter registration failed");
    require(!registry.registerConverter(std::make_shared<TestConverter>()), "Duplicate converter accepted");
    converter.reset();
    auto lease = registry.find(GADataObjectType::TriangleMesh, GADataObjectType::TriangleMesh);
    require(lease != nullptr, "Converter lookup failed");
    require(!registry.unregisterConverter(nullptr), "Null converter ID accepted");
    registry.clear();
    require(registry.size() == 0 && !registry.find(GADataObjectType::TriangleMesh, GADataObjectType::TriangleMesh),
            "Clear failed");
    require(!weak.expired() && std::string(lease->converterId()) == "test.mesh", "Clear invalidated lease");
    lease.reset();
    require(weak.expired(), "Converter leaked");
}
} // namespace

static_assert(!std::is_copy_constructible_v<IGAObject>);
static_assert(!std::is_move_constructible_v<IGAObject>);
static_assert(std::is_abstract_v<IGAAssembly<>>);

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Expected one test name");
        const std::string name = argv[1];
        if (name == "data")
            dataLifetime();
        else if (name == "mesh")
            meshOwnership();
        else if (name == "components")
            componentLifetime();
        else if (name == "converters")
            converterLifetime();
        else
            throw std::runtime_error("Unknown test");
        std::cout << name << " passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
