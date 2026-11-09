#include "GAFilePlugin.h"

#include <exception>

#if defined(_WIN32)
#define GA_TEST_EXPORT __declspec(dllexport)
#else
#define GA_TEST_EXPORT __attribute__((visibility("default")))
#endif

namespace
{
constexpr std::size_t CodecCount = 3;
int failureMode = 0;
int liveCodecs = 0;
int destroyedCodecs = 0;
int liveWhenExceptionDestroyed = -1;

class FactoryError : public std::exception
{
public:
    ~FactoryError() override
    {
        liveWhenExceptionDestroyed = liveCodecs;
    }
    const char* what() const noexcept override
    {
        return "Test plugin factory error";
    }
};

class TestCodec : public GAFileCodec
{
public:
    TestCodec()
    {
        ++liveCodecs;
    }
    ~TestCodec() override
    {
        --liveCodecs;
        ++destroyedCodecs;
    }
    const GAFileFormatDescriptor& descriptor() const noexcept override
    {
        static const GAFileFormatDescriptor format{"test.loader", "Loader test", nullptr, 0,
                                                    GAFileCapability::None, "test.loader", "Loader test"};
        return format;
    }
    bool acceptsExtension(const char*) const noexcept override
    {
        return false;
    }
    GAFileResult read(const GAFileReadRequest&, GADataStore&) override
    {
        return {};
    }
    GAFileResult write(const GAFileWriteRequest&, const GADataStore&) override
    {
        return {};
    }
};
} // namespace

extern "C" GA_TEST_EXPORT void gaTestSetFailure(int mode) noexcept
{
    failureMode = mode;
    destroyedCodecs = 0;
    liveWhenExceptionDestroyed = -1;
}
extern "C" GA_TEST_EXPORT int gaTestLiveCodecs() noexcept
{
    return liveCodecs;
}
extern "C" GA_TEST_EXPORT int gaTestDestroyedCodecs() noexcept
{
    return destroyedCodecs;
}
extern "C" GA_TEST_EXPORT int gaTestExceptionDestructionState() noexcept
{
    return liveWhenExceptionDestroyed;
}

extern "C" GA_TEST_EXPORT const GAFilePluginDescriptor* gaQueryFilePlugin() noexcept
{
    static const GAFilePluginDescriptor descriptor{GAFilePluginAbiVersion, "test.loader", "Loader test", 1,
                                                    CodecCount};
    return &descriptor;
}
extern "C" GA_TEST_EXPORT GAFileCodec* gaCreateFileCodec(std::size_t index)
{
    if (index >= CodecCount)
    {
        return nullptr;
    }
    if (index == 1)
    {
        if (failureMode == 1)
        {
            return nullptr;
        }
        if (failureMode == 2)
        {
            throw FactoryError{};
        }
        if (failureMode == 3)
        {
            throw 7;
        }
    }
    return new TestCodec;
}
extern "C" GA_TEST_EXPORT void gaDestroyFileCodec(GAFileCodec* codec) noexcept
{
    delete codec;
}
