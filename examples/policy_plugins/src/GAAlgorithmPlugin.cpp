#include "GAAlgorithmPlugin.h"

#include <QFileInfo>
#include <QLibrary>
#include <QString>

namespace ga::example
{
namespace
{
struct Module
{
    ~Module()
    {
        library.unload();
    }
    QLibrary library;
    std::string directory;
    GAQueryPlugin query = nullptr;
    GACreateAlgorithm create = nullptr;
    GADestroyAlgorithm destroy = nullptr;
};

class Factory : public IGAComponentFactory
{
public:
    Factory(std::shared_ptr<Module> module, GAComponentDescriptor descriptor)
        : module_(std::move(module))
        , descriptor_(std::move(descriptor))
    {
    }
    GAComponentDescriptor descriptor() const noexcept override
    {
        return descriptor_;
    }
    IGAObject* create() override
    {
        return module_->create(module_->directory.c_str());
    }
    void destroy(IGAObject* object) noexcept override
    {
        module_->destroy(object);
    }

private:
    std::shared_ptr<Module> module_;
    GAComponentDescriptor descriptor_;
};
} // namespace

class GAAlgorithmPluginLoader::Impl
{
public:
    IGAComponentRegistry* registry = nullptr;
    std::shared_ptr<IGAComponentFactory> factory;
    std::string engineId;
    std::string error;

    void detach() noexcept
    {
        // Do not remove a replacement factory registered by somebody else under the same ID.
        if (registry != nullptr && registry->find(engineId) == factory)
        {
            registry->unregisterFactory(engineId);
        }
        registry = nullptr;
        factory.reset();
        engineId.clear();
    }
};

GAAlgorithmPluginLoader::GAAlgorithmPluginLoader()
    : impl_(std::make_unique<Impl>())
{
}
GAAlgorithmPluginLoader::~GAAlgorithmPluginLoader()
{
    impl_->detach();
}

bool GAAlgorithmPluginLoader::load(std::string_view path, IGAComponentRegistry& registry)
{
    impl_->error.clear();
    if (impl_->registry != nullptr)
    {
        impl_->error = "Unload the current registration before loading another plugin";
        return false;
    }
    auto module = std::make_shared<Module>();
    const QFileInfo file(QString::fromUtf8(path.data(), static_cast<qsizetype>(path.size())));
    module->directory = file.absolutePath().toUtf8().toStdString();
    module->library.setFileName(file.absoluteFilePath());
    if (!module->library.load())
    {
        impl_->error = module->library.errorString().toStdString();
        return false;
    }
    module->query = reinterpret_cast<GAQueryPlugin>(module->library.resolve("gaQueryAlgorithmPlugin"));
    module->create = reinterpret_cast<GACreateAlgorithm>(module->library.resolve("gaCreateAlgorithm"));
    module->destroy = reinterpret_cast<GADestroyAlgorithm>(module->library.resolve("gaDestroyAlgorithm"));
    if (module->query == nullptr || module->create == nullptr || module->destroy == nullptr)
    {
        impl_->error = "Missing algorithm plugin entry point";
        return false;
    }
    const auto* descriptor = module->query();
    if (descriptor == nullptr || descriptor->abiVersion != GAExamplePluginAbiVersion ||
        descriptor->engine.id == nullptr || *descriptor->engine.id == '\0' || descriptor->engine.name == nullptr)
    {
        impl_->error = "Invalid algorithm plugin descriptor or ABI version";
        return false;
    }
    GAComponentDescriptor component{GAInvalidComponentId, descriptor->engine.id, descriptor->engine.name,
                                    descriptor->engine.version};
    impl_->engineId = component.typeName;
    auto factory = std::make_shared<Factory>(std::move(module), std::move(component));
    if (!registry.registerFactory(factory))
    {
        impl_->error = "An implementation with this engine ID is already registered";
        return false;
    }
    impl_->factory = std::move(factory);
    impl_->registry = &registry;
    return true;
}

void GAAlgorithmPluginLoader::unload(IGAComponentRegistry& registry) noexcept
{
    if (impl_->registry == &registry)
    {
        impl_->detach();
    }
}

const std::string& GAAlgorithmPluginLoader::error() const noexcept
{
    return impl_->error;
}

void GAAlgorithmDeleter::operator()(GAAlgorithmAssembly* object) const noexcept
{
    object->shutdown();
    factory->destroy(object);
}

GAAlgorithmInstance createAlgorithm(IGAComponentRegistry& registry, std::string_view engineId)
{
    auto factory = registry.find(engineId);
    GAAlgorithmInstance result{nullptr, GAAlgorithmDeleter{factory}};
    if (!factory)
    {
        return result;
    }
    auto* object = factory->create();
    auto* algorithm = dynamic_cast<GAAlgorithmAssembly*>(object);
    if (algorithm == nullptr)
    {
        if (object != nullptr)
        {
            factory->destroy(object);
        }
        return result;
    }
    result.reset(algorithm);
    if (!result->init())
    {
        result.reset();
    }
    return result;
}
} // namespace ga::example
