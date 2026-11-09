#include "GAAlgorithmPlugin.h"

#include <QCoreApplication>
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ga::example;

namespace
{
void check(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

// The workflow knows an operation and its interface, not the selected plugin class.
GAStatus runWorkflow(GAAlgorithmAssembly& engine, GAOperation operation, std::span<const GAPoint> input,
                     std::vector<GAPoint>& output)
{
    if (!engine.policy<GAAlgorithmPropertyPolicy>().supports(operation))
    {
        return GAStatus::Unsupported;
    }
    return engine.transform(operation, input, output);
}

struct Events : IGAAlgorithmObserver
{
    int started = 0;
    int completed = 0;
    int failed = 0;
    void onEvent(GAEvent event, GAStatus) noexcept override
    {
        if (event == GAEvent::Started)
        {
            ++started;
        }
        if (event == GAEvent::Completed)
        {
            ++completed;
        }
        if (event == GAEvent::Failed)
        {
            ++failed;
        }
    }
};
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    try
    {
        check(argc == 3, "Pass the reference and standard plugin library paths");
        // Registry outlives loaders. Components retain their own factory/library references.
        GAComponentRegistry registry;
        GAAlgorithmPluginLoader referenceLoader;
        GAAlgorithmPluginLoader standardLoader;
        check(referenceLoader.load(argv[1], registry), referenceLoader.error().c_str());
        check(standardLoader.load(argv[2], registry), standardLoader.error().c_str());
        check(registry.size() == 2, "Two engine factories should be registered");
        {
            GAAlgorithmPluginLoader duplicate;
            check(!duplicate.load(argv[1], registry), "Duplicate engine was accepted");
            check(registry.size() == 2, "Failed load changed registrations");
        }
        {
            GAAlgorithmPluginLoader missing;
            check(!missing.load("/missing/ga-example-plugin", registry), "Missing library was accepted");
        }
        check(!createAlgorithm(registry, "unknown.engine"), "Unknown selection fell back silently");

        // This choice can later come from options or from a workflow-specific selection.
        auto reference = createAlgorithm(registry, "example.reference");
        auto standard = createAlgorithm(registry, "example.standard");
        check(reference && standard, "Cannot obtain the algorithm interface");
        const std::array input{GAPoint{1.0, 2.0, 3.0}, GAPoint{-1.0, 0.0, 4.0}};
        const std::vector expected{GAPoint{2.0, 4.0, 6.0}, GAPoint{-2.0, 0.0, 8.0}};

        for (auto* engine : {reference.get(), standard.get()})
        {
            auto& properties = engine->policy<GAAlgorithmPropertyPolicy>();
            auto& persistence = engine->policy<GAAlgorithmSerializationPolicy>();
            auto& observation = engine->policy<GAAlgorithmObservationPolicy>();
            const auto info = properties.engineInfo();
            std::cout << info.id << ": scale=" << properties.supports(GAOperation::Scale)
                      << ", translate=" << properties.supports(GAOperation::Translate) << '\n';
            check(properties.setSettings({2.0, {1.0, 2.0, 3.0}}), "Settings rejected");
            const auto saved = persistence.saveSettings();
            check(properties.setSettings({3.0, {}}), "Temporary settings rejected");
            check(persistence.restoreSettings(saved), "Settings restore failed");
            check(properties.settings().scale == 2.0 && properties.settings().translation == GAPoint{1.0, 2.0, 3.0},
                  "Settings did not round-trip");
            check(!persistence.restoreSettings(saved + " garbage"), "Trailing state data accepted");
            check(!persistence.restoreSettings("ga.example.settings.v2 1 0 0 0"), "Unknown state version accepted");
            check(properties.settings().scale == 2.0, "Invalid restore changed state");
            Events events;
            observation.setObserver(events);
            std::vector<GAPoint> output;
            check(runWorkflow(*engine, GAOperation::Scale, input, output) == GAStatus::Success && output == expected,
                  "Same workflow produced incorrect results");
            check(events.started == 1 && events.completed == 1, "Missing operation notifications");
            const auto previous = output;
            check(properties.setSettings({std::numeric_limits<double>::max(), {}}), "Finite settings rejected");
            check(runWorkflow(*engine, GAOperation::Scale, input, output) == GAStatus::InvalidInput &&
                      output == previous,
                  "Overflow changed output");
            check(events.failed == 1, "Missing failure notification");
            check(persistence.restoreSettings(saved), "Cannot restore usable settings");
            Events replacement;
            observation.setObserver(replacement);
            check(runWorkflow(*engine, GAOperation::Scale, input, output) == GAStatus::Success,
                  "Replacement observer operation failed");
            check(replacement.completed == 1 && events.completed == 1, "Observer replacement failed");
            observation.clearObserver();
            check(runWorkflow(*engine, GAOperation::Scale, input, output) == GAStatus::Success &&
                      replacement.completed == 1,
                  "Cleared observer received another event");
        }

        std::vector<GAPoint> output{GAPoint{9.0, 9.0, 9.0}};
        const auto previous = output;
        check(runWorkflow(*standard, GAOperation::Translate, input, output) == GAStatus::Unsupported &&
                  output == previous,
              "Workflow ignored unsupported operation");
        check(runWorkflow(*reference, GAOperation::Translate, input, output) == GAStatus::Success &&
                  output.front() == GAPoint{2.0, 4.0, 6.0},
              "Reference translation failed");

        // Removing discovery entries must not unload code still needed by a live component.
        referenceLoader.unload(registry);
        standardLoader.unload(registry);
        check(registry.size() == 0 && !createAlgorithm(registry, "example.reference"), "Unregistration failed");
        check(runWorkflow(*reference, GAOperation::Scale, input, output) == GAStatus::Success && output == expected,
              "Live reference component lost its plugin");
        check(runWorkflow(*standard, GAOperation::Scale, input, output) == GAStatus::Success && output == expected,
              "Live standard component lost its plugin");
        reference.reset();
        standard.reset();
        check(referenceLoader.load(argv[1], registry), "Plugin could not be registered again");
        auto reloaded = createAlgorithm(registry, "example.reference");
        check(reloaded && reloaded->settings().scale == 1.0, "New instance inherited old settings");
        reloaded->shutdown();
        check(runWorkflow(*reloaded, GAOperation::Scale, input, output) == GAStatus::NotInitialized,
              "Stopped component accepted work");
        std::cout << "Policy composition, backend selection, persistence, events and lifetime checks passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
