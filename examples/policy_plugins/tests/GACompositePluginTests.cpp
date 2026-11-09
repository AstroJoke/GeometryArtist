#include "GAAlgorithmPlugin.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace ga::example;
namespace fs = std::filesystem;

namespace
{
void check(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

struct CompletionObserver : IGAAlgorithmObserver
{
    int completed = 0;
    void onEvent(GAEvent event, GAStatus) noexcept override
    {
        if (event == GAEvent::Completed)
        {
            ++completed;
        }
    }
};

void runWorkflow(GAAlgorithmAssembly& engine)
{
    auto& properties = engine.policy<GAAlgorithmPropertyPolicy>();
    auto& persistence = engine.policy<GAAlgorithmSerializationPolicy>();
    auto& observation = engine.policy<GAAlgorithmObservationPolicy>();
    check(properties.supports(GAOperation::Scale) && properties.supports(GAOperation::Translate),
          "Composed engine lost its capabilities");
    check(properties.setSettings({2.0, {1.0, 2.0, 3.0}}), "Settings rejected");
    const auto saved = persistence.saveSettings();
    check(properties.setSettings({3.0, {}}) && persistence.restoreSettings(saved), "Persistence failed");
    CompletionObserver observer;
    observation.setObserver(observer);
    const std::array input{GAPoint{1.0, 2.0, 3.0}, GAPoint{-1.0, 0.0, 4.0}};
    std::vector<GAPoint> output;
    check(engine.transform(GAOperation::Scale, input, output) == GAStatus::Success &&
              output == std::vector{GAPoint{2.0, 4.0, 6.0}, GAPoint{-2.0, 0.0, 8.0}},
          "Composed scale produced incorrect results");
    check(engine.transform(GAOperation::Translate, input, output) == GAStatus::Success &&
              output == std::vector{GAPoint{2.0, 4.0, 6.0}, GAPoint{0.0, 2.0, 7.0}},
          "Composed translation produced incorrect results");
    observation.clearObserver();
    check(observer.completed == 2, "Child notifications were not forwarded");
    check(engine.transform(GAOperation::Scale, input, output) == GAStatus::Success && observer.completed == 2,
          "Child retained a cleared observer");
}

void testPackaging(const fs::path& path)
{
    GAComponentRegistry registry;
    GAAlgorithmPluginLoader loader;
    check(loader.load(path.string(), registry), loader.error().c_str());
    auto engine = createAlgorithm(registry, "example.composed");
    check(static_cast<bool>(engine), "Cannot initialize composed engine");
    check(registry.size() == 1 && !registry.find("example.reference"), "Private child leaked into host registry");
    runWorkflow(*engine);
    loader.unload(registry);
    check(registry.size() == 0, "Parent registration was not removed");
    runWorkflow(*engine);
    engine->shutdown();
    check(!engine->isInitialized(), "Parent did not stop");
    const std::array input{GAPoint{1.0, 2.0, 3.0}};
    std::vector<GAPoint> output;
    check(engine->transform(GAOperation::Scale, input, output) == GAStatus::NotInitialized,
          "Stopped parent accepted work");
    check(engine->init(), "Parent could not recreate its dependency");
    runWorkflow(*engine);
}

void testMissingAndWrongChild(const fs::path& parent, const fs::path& child, const fs::path& wrongChild)
{
    QTemporaryDir temporary;
    check(temporary.isValid(), "Cannot create isolated plugin directory");
    const fs::path directory(temporary.path().toStdString());
    const auto parentCopy = directory / parent.filename();
    const auto childCopy = directory / child.filename();
    fs::copy_file(parent, parentCopy);
    GAComponentRegistry registry;
    GAAlgorithmPluginLoader loader;
    check(loader.load(parentCopy.string(), registry), loader.error().c_str());
    check(!createAlgorithm(registry, "example.composed"), "Missing child silently accepted");
    check(registry.size() == 1, "Missing child changed host registrations");
    fs::copy_file(wrongChild, childCopy);
    check(!createAlgorithm(registry, "example.composed"), "Unexpected child engine silently accepted");
    check(registry.size() == 1, "Wrong child changed host registrations");
    fs::remove(childCopy);
    fs::copy_file(child, childCopy);
    auto engine = createAlgorithm(registry, "example.composed");
    check(static_cast<bool>(engine), "Initialization did not recover after dependency repair");
    runWorkflow(*engine);
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    try
    {
        check(argc == 5, "Pass composite, bundled, reference and standard plugin paths");
        // Check isolated failures before loading the dependency through any other path.
        testMissingAndWrongChild(argv[1], argv[3], argv[4]);
        testPackaging(argv[1]);
        QTemporaryDir temporary;
        check(temporary.isValid(), "Cannot create bundled plugin directory");
        const auto bundled = fs::path(temporary.path().toStdString()) / fs::path(argv[2]).filename();
        fs::copy_file(argv[2], bundled);
        testPackaging(bundled);
        std::cout << "Private dependency loading, failure recovery, lifetime and bundled packaging passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
