#include "GAFilePluginLoader.h"

#include <QCoreApplication>
#include <QLibrary>
#include <iostream>
#include <stdexcept>

namespace
{
void check(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    try
    {
        check(argc == 2, "Expected failure plugin path");
        // Keep test counters available after the loader releases its own library reference.
        QLibrary monitor(QString::fromUtf8(argv[1]));
        check(monitor.load(), "Cannot load failure plugin monitor");
        const auto setFailure = reinterpret_cast<void (*)(int) noexcept>(monitor.resolve("gaTestSetFailure"));
        const auto live = reinterpret_cast<int (*)() noexcept>(monitor.resolve("gaTestLiveCodecs"));
        const auto destroyed = reinterpret_cast<int (*)() noexcept>(monitor.resolve("gaTestDestroyedCodecs"));
        const auto exceptionState =
            reinterpret_cast<int (*)() noexcept>(monitor.resolve("gaTestExceptionDestructionState"));
        check(setFailure && live && destroyed && exceptionState, "Missing test entry point");
        GAFilePluginLoader loader;
        for (int failure : {1, 2, 3})
        {
            setFailure(failure);
            check(!loader.load(argv[1]), "Incomplete plugin load was accepted");
            check(!loader.isLoaded() && loader.descriptor() == nullptr && loader.codecCount() == 0 &&
                      loader.codec(0) == nullptr,
                  "Failed plugin load retained discoverable objects");
            check(live() == 0 && destroyed() == 1, "Failed plugin load did not roll back created codecs");
            check(!loader.errorString().empty(), "Failed plugin load lost its diagnostic");
            if (failure == 2)
            {
                check(exceptionState() == 1, "Plugin exception outlived rollback and library release");
            }
            loader.unload();
            check(destroyed() == 1, "Repeated unload destroyed a codec twice");

            setFailure(0);
            check(loader.load(argv[1]) && loader.isLoaded() && loader.codecCount() == 3 && live() == 3,
                  "Loader could not recover after creation failure");
            check(loader.errorString().empty(), "Successful reload retained the old error");
            loader.unload();
            check(live() == 0 && destroyed() == 3, "Successful load did not release all codecs");
        }
        std::cout << "Factory failure rollback, exception lifetime and loader recovery passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
