#include "GACgalGeometryAdapter.h"
#include "GACgalGeometryTestMeshes.h"
#include "GADataStore.h"

#include <barrier>
#include <chrono>
#include <future>
#include <iostream>
#include <vector>

using namespace ga::cgal_test;

namespace
{
double volume(GACgalGeometryAdapter& resource, GAGeometryHandle handle)
{
    GADataStore store;
    const auto data = resource.exportGeometry(handle, "copy", store);
    auto mesh = std::dynamic_pointer_cast<const GAMeshDataObject>(store.access(data));
    require(mesh != nullptr, "Copy export failed");
    return signedVolume(*mesh);
}

void oppositeCopies()
{
    GACgalGeometryAdapter a, b;
    const auto x = a.importGeometry(*cube(0, 1)), y = b.importGeometry(*cube(2, 3));
    std::barrier start(2);
    const auto copy = [&](GACgalGeometryAdapter& destination, const GACgalGeometryAdapter& source, GAGeometryHandle h)
    {
        start.arrive_and_wait();
        constexpr unsigned Iterations = 16;
        for (unsigned i = 0; i < Iterations; ++i)
        {
            const auto result = destination.copyGeometryFrom(source, h);
            require(result.status == GAGeometryStatus::Success, "Opposite copy failed");
            destination.releaseGeometry(result.output);
        }
    };
    auto first = std::async(std::launch::async,
                            [&]
                            {
                                copy(a, b, y);
                            });
    auto second = std::async(std::launch::async,
                             [&]
                             {
                                 copy(b, a, x);
                             });
    constexpr auto Timeout = std::chrono::seconds(10);
    require(first.wait_for(Timeout) == std::future_status::ready &&
                second.wait_for(Timeout) == std::future_status::ready,
            "Opposite copies deadlocked");
    first.get();
    second.get();
    require(a.resourceCount() == 1 && b.resourceCount() == 1, "Opposite copies leaked handles");
}
void copiesPreserveExactState()
{
    GACgalGeometryAdapter source({3}), destination({1});
    const auto input = source.importGeometry(*cube(0, 1.0 / 3.0));
    require(source.isValid(input), "Rational source import failed");
    const auto copy = destination.copyGeometryFrom(source, input);
    require(copy.status == GAGeometryStatus::Success && destination.isValid(copy.output) &&
                !source.isValid(copy.output),
            "Copy has wrong ownership");
    const auto roundTrip = source.copyGeometryFrom(destination, copy.output);
    require(roundTrip.status == GAGeometryStatus::Success, "Exact copy round trip failed");
    const auto diff = source.apply(GABooleanOperation::Difference, input, roundTrip.output);
    const auto reverse = source.apply(GABooleanOperation::Difference, roundTrip.output, input);
    require(diff.status == GAGeometryStatus::Success && reverse.status == GAGeometryStatus::Success &&
                volume(source, diff.output) == 0 && volume(source, reverse.output) == 0,
            "Rational geometry changed during copy");
    source.releaseGeometry(input);
    require(std::abs(volume(destination, copy.output) - 1.0 / 27.0) < VolumeTolerance,
            "Copy was re-quantized or depends on source lifetime");
    const auto self = destination.copyGeometryFrom(destination, copy.output);
    require(self.status == GAGeometryStatus::Success && self.output != copy.output, "Self copy failed");
    GATransform move;
    move.values[3] = 5;
    require(destination.transform(copy.output, move) == GAGeometryStatus::Success, "Copy mutation failed");
    const auto separate = destination.apply(GABooleanOperation::Intersection, self.output, copy.output);
    require(separate.status == GAGeometryStatus::Success && volume(destination, separate.output) == 0,
            "Copied resources alias mutable geometry");
    const auto before = destination.resourceCount();
    const auto failed = destination.copyGeometryFrom(source, input);
    require(failed.status == GAGeometryStatus::InvalidArgument && failed.output == GAInvalidGeometryHandle &&
                destination.resourceCount() == before,
            "Failed copy published a resource");
    const auto emptyCopy = destination.copyGeometryFrom(source, diff.output);
    require(emptyCopy.status == GAGeometryStatus::Success && volume(destination, emptyCopy.output) == 0,
            "Empty copy failed");
}

void independentWorkers()
{
    GACgalGeometryAdapter source;
    const auto outer = source.importGeometry(*cube(0, 4));
    const auto inner = source.importGeometry(*cube(1, 3));
    const auto hollow = source.apply(GABooleanOperation::Difference, outer, inner);
    require(hollow.status == GAGeometryStatus::Success, "Hollow copy fixture failed");
    constexpr int WorkerCount = 4;
    std::vector<std::future<double>> jobs;
    for (int i = 0; i < WorkerCount; ++i)
    {
        jobs.push_back(
            std::async(std::launch::async,
                       [&source, hollow]
                       {
                           GACgalGeometryAdapter worker;
                           const auto copy = worker.copyGeometryFrom(source, hollow.output);
                           require(copy.status == GAGeometryStatus::Success, "Worker snapshot failed");
                           const auto removed = worker.apply(GABooleanOperation::Difference, copy.output, copy.output);
                           require(removed.status == GAGeometryStatus::Success && volume(worker, removed.output) == 0,
                                   "Private worker Boolean failed");
                           return volume(worker, copy.output);
                       }));
    }
    for (auto& job : jobs)
    {
        require(std::abs(job.get() - 56) < VolumeTolerance, "Worker copy lost cavity");
    }
    require(std::abs(volume(source, hollow.output) - 56) < VolumeTolerance, "Workers changed source geometry");
}
} // namespace

int main()
{
    try
    {
        oppositeCopies();
        copiesPreserveExactState();
        independentWorkers();
        std::cout << "Independent exact resource copies passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
