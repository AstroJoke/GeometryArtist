#include "GAFilePluginLoader.h"

#include <QLibrary>
#include <vector>

class GAFilePluginLoader::Impl
{
public:
    ~Impl()
    {
        close();
    }

    void close() noexcept
    {
        // Destruction order is part of the ABI contract: each codec executes
        // plugin code while being destroyed, so the library must remain loaded.
        if (destroy != nullptr)
        {
            for (GAFileCodec* codec : codecs)
            {
                destroy(codec);
            }
        }
        codecs.clear();
        descriptor = nullptr;
        query = nullptr;
        create = nullptr;
        destroy = nullptr;
        if (library.isLoaded())
        {
            library.unload();
        }
    }

    QLibrary library;
    GAQueryFilePluginFunction query = nullptr;
    GACreateFileCodecFunction create = nullptr;
    GADestroyFileCodecFunction destroy = nullptr;
    const GAFilePluginDescriptor* descriptor = nullptr;
    std::vector<GAFileCodec*> codecs;
    std::string error;
};

GAFilePluginLoader::GAFilePluginLoader()
    : impl_(std::make_unique<Impl>())
{
}

GAFilePluginLoader::~GAFilePluginLoader() = default;
GAFilePluginLoader::GAFilePluginLoader(GAFilePluginLoader&&) noexcept = default;
GAFilePluginLoader& GAFilePluginLoader::operator=(GAFilePluginLoader&&) noexcept = default;

bool GAFilePluginLoader::load(const std::string& filePath)
{
    impl_->close();
    impl_->error.clear();
    impl_->library.setFileName(QString::fromStdString(filePath));
    if (!impl_->library.load())
    {
        impl_->error = impl_->library.errorString().toStdString();
        return false;
    }

    // Resolve only the stable C entry points. C++ class names and implementation
    // details are intentionally not discovered through the dynamic loader.
    impl_->query = reinterpret_cast<GAQueryFilePluginFunction>(impl_->library.resolve("gaQueryFilePlugin"));
    impl_->create = reinterpret_cast<GACreateFileCodecFunction>(impl_->library.resolve("gaCreateFileCodec"));
    impl_->destroy = reinterpret_cast<GADestroyFileCodecFunction>(impl_->library.resolve("gaDestroyFileCodec"));
    if (impl_->query == nullptr || impl_->create == nullptr || impl_->destroy == nullptr)
    {
        impl_->error = "File plugin does not provide the required entry points";
        impl_->close();
        return false;
    }

    impl_->descriptor = impl_->query();
    if (impl_->descriptor == nullptr || impl_->descriptor->abiVersion != GAFilePluginAbiVersion)
    {
        impl_->error = "File plugin ABI version mismatch";
        impl_->close();
        return false;
    }

    try
    {
        for (std::size_t index = 0; index < impl_->descriptor->codecCount; ++index)
        {
            // The pending codec is owned even if growing the container throws.
            std::unique_ptr<GAFileCodec, GADestroyFileCodecFunction> codec(impl_->create(index), impl_->destroy);
            if (!codec)
            {
                impl_->close();
                impl_->error = "File plugin failed to create a codec";
                return false;
            }
            impl_->codecs.push_back(codec.get());
            codec.release();
        }
        return true;
    }
    catch (...)
    {
        // Leave the handler before unloading: the exception may be defined by the plugin.
    }
    impl_->close();
    impl_->error = "File plugin codec creation threw an exception";
    return false;
}

void GAFilePluginLoader::unload() noexcept
{
    impl_->close();
}

bool GAFilePluginLoader::isLoaded() const noexcept
{
    return impl_->descriptor != nullptr;
}

const GAFilePluginDescriptor* GAFilePluginLoader::descriptor() const noexcept
{
    return impl_->descriptor;
}

std::size_t GAFilePluginLoader::codecCount() const noexcept
{
    return impl_->codecs.size();
}

GAFileCodec* GAFilePluginLoader::codec(std::size_t index) noexcept
{
    return index < impl_->codecs.size() ? impl_->codecs[index] : nullptr;
}

const std::string& GAFilePluginLoader::errorString() const noexcept
{
    return impl_->error;
}
