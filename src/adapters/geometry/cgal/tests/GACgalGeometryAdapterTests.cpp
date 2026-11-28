#include "GACgalGeometryAdapter.h"
#include "GACgalGeometryTestMeshes.h"
#include "GADataStore.h"

#include <algorithm>
#include <future>
#include <iostream>
#include <limits>

using namespace ga::cgal_test;

namespace
{
double volume(GACgalGeometryAdapter& adapter, GAGeometryHandle handle)
{
    GADataStore store;
    const auto snapshot = adapter.exportGeometry(handle, "test", store);
    require(snapshot != GAInvalidDataHandle, "Export failed");
    const auto data = std::dynamic_pointer_cast<const GAMeshDataObject>(store.access(snapshot));
    require(data != nullptr, "Not a mesh snapshot");
    return signedVolume(*data);
}

void expectVolume(GACgalGeometryAdapter& adapter, GAGeometryHandle handle, double expected)
{
    const auto actual = volume(adapter, handle);
    if (std::abs(actual - expected) >= VolumeTolerance)
    {
        throw std::runtime_error("Unexpected solid volume: expected " + std::to_string(expected) + ", actual " +
                                 std::to_string(actual));
    }
}

void booleanAndLifetime()
{
    GACgalGeometryAdapter adapter;
    auto stock = cube();
    auto tool = cylinder();
    const auto a = adapter.importGeometry(*stock), b = adapter.importGeometry(*tool);
    require(a != GAInvalidGeometryHandle && b != GAInvalidGeometryHandle, "Fixture import failed");
    require(adapter.representation(a) == GAGeometryRepresentation::BRep, "Wrong representation");
    // Exact resources survive destruction of their neutral input data.
    stock.reset();
    tool.reset();
    const double area = cylinderArea();
    const std::pair<GABooleanOperation, double> cases[] = {{GABooleanOperation::Difference, 8 - 2 * area},
                                                           {GABooleanOperation::Union, 8 + area},
                                                           {GABooleanOperation::Intersection, 2 * area}};
    for (const auto [operation, expected] : cases)
    {
        const auto result = adapter.apply(operation, a, b);
        require(result.status == GAGeometryStatus::Success, "Boolean failed");
        require(result.output != a && result.output != b, "Output aliases an input handle");
        expectVolume(adapter, result.output, expected);
        expectVolume(adapter, a, 8);
        expectVolume(adapter, b, 3 * area);
        adapter.releaseGeometry(result.output);
        require(!adapter.isValid(result.output), "Released output is still valid");
    }
    auto difference = adapter.apply(GABooleanOperation::Difference, a, b);
    // Chain native output directly, without a double reimport; repeated difference is idempotent.
    for (int step = 0; step < 3; ++step)
    {
        const auto next = adapter.apply(GABooleanOperation::Difference, difference.output, b);
        require(next.status == GAGeometryStatus::Success, "Chained difference failed");
        adapter.releaseGeometry(difference.output);
        difference = next;
    }
    adapter.releaseGeometry(a);
    adapter.releaseGeometry(b);
    expectVolume(adapter, difference.output, 8 - 2 * area);
    const auto saved = difference.output;
    adapter.releaseGeometry(saved);
    adapter.releaseGeometry(saved);
    require(adapter.resourceCount() == 0, "Resource leak");
    const auto fresh = adapter.importGeometry(*cube());
    require(fresh != saved && !adapter.isValid(saved), "A stale handle was reused");
    GACgalGeometryAdapter other;
    const auto foreign = other.importGeometry(*cube());
    require(!adapter.isValid(foreign), "Foreign handle accepted");
    require(adapter.apply(GABooleanOperation::Difference, fresh, foreign).status == GAGeometryStatus::InvalidArgument,
            "Foreign Boolean input accepted");
}

void emptyAndContact()
{
    GACgalGeometryAdapter adapter;
    const auto a = adapter.importGeometry(*cube());
    const auto removed = adapter.apply(GABooleanOperation::Difference, a, a);
    require(removed.status == GAGeometryStatus::Success && adapter.isValid(removed.output), "Empty result rejected");
    expectVolume(adapter, removed.output, 0);
    GADataStore store;
    const auto emptyData = adapter.exportGeometry(removed.output, nullptr, store);
    const auto emptyObject = std::dynamic_pointer_cast<const GAMeshDataObject>(store.access(emptyData));
    require(emptyObject && emptyObject->vertexCount() == 0 && emptyObject->indexCount() == 0, "Not an empty mesh");
    require(adapter.isValid(adapter.importGeometry(*emptyObject)), "Empty import failed");
    const auto restored = adapter.apply(GABooleanOperation::Union, removed.output, a);
    require(restored.status == GAGeometryStatus::Success, "Union with empty failed");
    expectVolume(adapter, restored.output, 8);
    const auto disjoint = adapter.importGeometry(*cube(3, 4));
    const auto none = adapter.apply(GABooleanOperation::Intersection, a, disjoint);
    require(none.status == GAGeometryStatus::Success, "Disjoint intersection failed");
    expectVolume(adapter, none.output, 0);
    auto adjacent = cube();
    auto view = adjacent->view();
    std::vector<double> positions(view.positions, view.positions + view.vertexCount * CoordinateCount);
    for (std::size_t i = 0; i < positions.size(); i += CoordinateCount)
    {
        positions[i] += 2;
    }
    auto touching = mesh(positions, {view.indices, view.indices + view.indexCount});
    const auto contact = adapter.importGeometry(*touching);
    const auto plane = adapter.apply(GABooleanOperation::Intersection, a, contact);
    require(plane.status == GAGeometryStatus::Success, "Contact regularization failed");
    expectVolume(adapter, plane.output, 0);
}

void failureAtomicity()
{
    GACgalGeometryAdapter adapter;
    const auto a = adapter.importGeometry(*cube());
    const auto count = adapter.resourceCount();
    GAGeometryOptions options;
    options.tolerance = 0.001;
    require(adapter.apply(GABooleanOperation::Difference, a, a, options).status == GAGeometryStatus::Unsupported,
            "Tolerance silently ignored");
    options.tolerance = std::numeric_limits<double>::quiet_NaN();
    require(adapter.apply(GABooleanOperation::Difference, a, a, options).status == GAGeometryStatus::InvalidArgument,
            "NaN tolerance accepted");
    options = {};
    options.repairInput = true;
    require(adapter.apply(GABooleanOperation::Difference, a, a, options).status == GAGeometryStatus::Unsupported,
            "Implicit repair accepted");
    const auto invalid = adapter.apply(GABooleanOperation::Union, a, GAInvalidGeometryHandle);
    require(invalid.status == GAGeometryStatus::InvalidArgument && invalid.output == GAInvalidGeometryHandle,
            "Invalid input published an output");
    require(adapter.apply(static_cast<GABooleanOperation>(255), a, a).status == GAGeometryStatus::Unsupported,
            "Unknown operation accepted");
    GADataStore store;
    require(adapter.exportGeometry(GAInvalidGeometryHandle, "invalid", store) == GAInvalidDataHandle &&
                store.size() == 0,
            "Invalid export changed data store");
    require(adapter.resourceCount() == count, "Failure published a resource");
    expectVolume(adapter, a, 8);
    const auto touching = adapter.importGeometry(*cube(2, 4));
    const auto before = adapter.resourceCount();
    const auto nonManifold = adapter.apply(GABooleanOperation::Union, a, touching);
    require(nonManifold.status == GAGeometryStatus::InvalidGeometry && nonManifold.output == GAInvalidGeometryHandle &&
                adapter.resourceCount() == before,
            "Non-manifold output was published");
    expectVolume(adapter, a, 8);
}

void importValidationAndPrecision()
{
    GACgalGeometryAdapter adapter;
    auto original = cube();
    const auto view = original->view();
    std::vector<double> points(view.positions, view.positions + view.vertexCount * CoordinateCount);
    std::vector<std::uint32_t> triangles(view.indices, view.indices + view.indexCount);
    auto open = triangles;
    open.resize(open.size() - TriangleCornerCount);
    require(adapter.importGeometry(*mesh(points, open)) == GAInvalidGeometryHandle, "Open mesh accepted");
    auto inward = triangles;
    for (std::size_t i = 0; i < inward.size(); i += TriangleCornerCount)
    {
        std::swap(inward[i], inward[i + 1]);
    }
    require(adapter.importGeometry(*mesh(points, inward)) == GAInvalidGeometryHandle, "Inward solid accepted");
    auto invalidPoints = points;
    invalidPoints[0] = std::numeric_limits<double>::infinity();
    require(adapter.importGeometry(*mesh(invalidPoints, triangles)) == GAInvalidGeometryHandle, "Infinity accepted");
    auto degenerate = triangles;
    degenerate[0] = degenerate[1];
    require(adapter.importGeometry(*mesh(points, degenerate)) == GAInvalidGeometryHandle,
            "Degenerate triangle accepted");
    auto crossed = points;
    crossed[6 * CoordinateCount] = -1;
    crossed[6 * CoordinateCount + 1] = -1;
    crossed[6 * CoordinateCount + 2] = -1;
    require(adapter.importGeometry(*mesh(crossed, triangles)) == GAInvalidGeometryHandle, "Self intersection accepted");
    require(adapter.resourceCount() == 0, "Rejected import leaked a resource");

    // Cavities require reversed inner shells; not every inward component is invalid.
    auto outer = cube(0, 4);
    auto inner = cube(1, 3);
    const auto outerView = outer->view(), innerView = inner->view();
    std::vector<double> hollowPoints(outerView.positions,
                                     outerView.positions + outerView.vertexCount * CoordinateCount);
    hollowPoints.insert(hollowPoints.end(), innerView.positions,
                        innerView.positions + innerView.vertexCount * CoordinateCount);
    std::vector<std::uint32_t> hollowTriangles(outerView.indices, outerView.indices + outerView.indexCount);
    const auto offset = static_cast<std::uint32_t>(outerView.vertexCount);
    for (std::size_t i = 0; i < innerView.indexCount; i += TriangleCornerCount)
    {
        hollowTriangles.insert(hollowTriangles.end(), {offset + innerView.indices[i + 1], offset + innerView.indices[i],
                                                       offset + innerView.indices[i + 2]});
    }
    const auto hollow = adapter.importGeometry(*mesh(hollowPoints, hollowTriangles));
    require(adapter.isValid(hollow), "Valid hollow solid rejected");
    expectVolume(adapter, hollow, 56);
    adapter.releaseGeometry(hollow);
    for (const auto limits : {std::pair{1.5, 2.5}, std::pair{5.0, 6.0}})
    {
        const auto additional = cube(limits.first, limits.second);
        const auto additionalView = additional->view();
        const auto base = static_cast<std::uint32_t>(hollowPoints.size() / CoordinateCount);
        hollowPoints.insert(hollowPoints.end(), additionalView.positions,
                            additionalView.positions + additionalView.vertexCount * CoordinateCount);
        for (std::size_t i = 0; i < additionalView.indexCount; ++i)
        {
            hollowTriangles.push_back(base + additionalView.indices[i]);
        }
    }
    const auto nested = adapter.importGeometry(*mesh(hollowPoints, hollowTriangles));
    require(adapter.isValid(nested), "Nested island/disconnected solid rejected");
    expectVolume(adapter, nested, 58);
    adapter.releaseGeometry(nested);

    constexpr std::uint32_t GridDenominator = 1000;
    GACgalGeometryAdapter quantized({GridDenominator});
    const auto exact = adapter.importGeometry(*cube(0.0004, 1.0004));
    const auto snapped = quantized.importGeometry(*cube(0.0004, 1.0004));
    require(adapter.isValid(exact) && quantized.isValid(snapped), "Precision import failed");
    GADataStore store;
    const auto getMin = [&](GACgalGeometryAdapter& resource, GAGeometryHandle handle)
    {
        const auto id = resource.exportGeometry(handle, "precision", store);
        auto object = std::dynamic_pointer_cast<const GAMeshDataObject>(store.access(id));
        require(object != nullptr, "Precision export failed");
        const auto data = object->view();
        return *std::min_element(data.positions, data.positions + data.vertexCount * CoordinateCount);
    };
    require(std::abs(getMin(adapter, exact) - 0.0004) < VolumeTolerance, "Default import rounded input");
    require(getMin(quantized, snapped) == 0, "Explicit grid was not applied");
    require(quantized.importGeometry(*cube(0, 0.0001)) == GAInvalidGeometryHandle, "Collapsed grid mesh accepted");
    require(quantized.importGeometry(*cube(1e14, 1e14 + 1)) == GAInvalidGeometryHandle, "Grid overflow accepted");
}

void concurrentCalls()
{
    GACgalGeometryAdapter adapter;
    const auto a = adapter.importGeometry(*cube());
    constexpr int WorkerCount = 4;
    std::vector<std::future<GAGeometryResult>> jobs;
    for (int worker = 0; worker < WorkerCount; ++worker)
    {
        jobs.push_back(std::async(std::launch::async,
                                  [&]
                                  {
                                      return adapter.apply(GABooleanOperation::Difference, a, a);
                                  }));
    }
    for (auto& job : jobs)
    {
        const auto result = job.get();
        require(result.status == GAGeometryStatus::Success, "Concurrent call failed");
        expectVolume(adapter, result.output, 0);
        adapter.releaseGeometry(result.output);
    }
    require(adapter.resourceCount() == 1, "Concurrent lifecycle leaked");
}
} // namespace

int main()
{
    try
    {
        booleanAndLifetime();
        emptyAndContact();
        failureAtomicity();
        importValidationAndPrecision();
        concurrentCalls();
        std::cout << "CGAL adapter: Boolean volumes, ownership, chaining, empty/contact, failures, precision and "
                     "concurrency passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
