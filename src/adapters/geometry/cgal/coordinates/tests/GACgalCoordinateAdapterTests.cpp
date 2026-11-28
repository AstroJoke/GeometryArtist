#include "GACgalCoordinateAdapter.h"
#include "GACgalGeometryAdapter.h"
#include "GACgalGeometryTestMeshes.h"
#include "GADataStore.h"

#include <algorithm>
#include <iostream>
#include <limits>

using namespace ga::cgal_test;
using namespace ga::coordinates;

namespace
{
bool empty(GACgalGeometryAdapter& geometry, GAGeometryHandle handle)
{
    GADataStore store;
    const auto data = geometry.exportGeometry(handle, "check", store);
    auto mesh = std::dynamic_pointer_cast<const GAMeshDataObject>(store.access(data));
    require(mesh != nullptr, "Placement result export failed");
    return mesh->empty();
}

void sameSet(GACgalGeometryAdapter& geometry, GAGeometryHandle left, GAGeometryHandle right)
{
    const auto a = geometry.apply(GABooleanOperation::Difference, left, right);
    const auto b = geometry.apply(GABooleanOperation::Difference, right, left);
    require(a.status == GAGeometryStatus::Success && b.status == GAGeometryStatus::Success &&
                empty(geometry, a.output) && empty(geometry, b.output),
            "Exact placed sets differ");
    geometry.releaseGeometry(a.output);
    geometry.releaseGeometry(b.output);
}

void explicitInputPrecision()
{
    GACgalGeometryAdapter geometry;
    GACgalCoordinateAdapter coordinates(geometry);
    const auto input = geometry.importGeometry(*cube());
    FrameTransformSnapshot path;
    path.steps = {{{{1.26, -0.25, 0.74}, {0.3, -0.2, 0.7, 0.9}}, false, {2, 1000}}};
    const auto placed = coordinates.createPlaced(input, path);
    require(placed.status == GAGeometryStatus::Success, "Explicit precision placement failed");
    const auto q = normalized(path.steps.front().localToParent.rotation);
    FrameTransformSnapshot reference;
    // Integer quaternion numerators represent the same rational rotation without a rounded double division.
    reference.steps = {
        {{{1.5, -0.5, 0.5},
          {std::round(q.x * 1000), std::round(q.y * 1000), std::round(q.z * 1000), std::round(q.w * 1000)}},
         false}};
    const auto expected = coordinates.createPlaced(input, reference);
    require(expected.status == GAGeometryStatus::Success, "Precision reference failed");
    sameSet(geometry, placed.output, expected.output);
    const auto display = path.approximatePose();
    require(display.translation.x == 1.5 && display.translation.y == -0.5 && display.translation.z == 0.5,
            "Display ignored input precision");
    path.steps.front().inverted = true;
    const auto restored = coordinates.createPlaced(placed.output, path);
    require(restored.status == GAGeometryStatus::Success, "Quantized inverse failed");
    sameSet(geometry, input, restored.output);
    path.steps.front().localToParent.translation.x = 1e20;
    require(coordinates.createPlaced(input, path).status == GAGeometryStatus::InvalidArgument,
            "Out-of-range quantized translation accepted");
}

void exactRoundTrip()
{
    GACgalGeometryAdapter geometry;
    GACgalCoordinateAdapter coordinates(geometry);
    FrameGraph graph(LengthUnit::Millimetre);
    const auto machine = graph.addFrame(kWorldFrame, {{1e16, -1e16, 1e16}, {}});
    const auto stock = graph.addFrame(machine, {{10, 20, 30}, {0.3, -0.2, 0.7, 0.9}});
    const auto tool = graph.addFrame(machine, {{-3, 1, 2}, {-0.1, 0.6, 0.2, 0.5}});
    const auto snapshot = graph.snapshot(tool, stock);
    require(snapshot.steps.size() == 2, "Shared ancestry was not cancelled");
    const auto input = geometry.importGeometry(*cube());
    const auto placed = coordinates.createPlaced(input, snapshot);
    require(placed.status == GAGeometryStatus::Success && placed.output != input, "Tool-to-stock placement failed");
    const auto restored = coordinates.createPlaced(placed.output, graph.snapshot(stock, tool));
    require(restored.status == GAGeometryStatus::Success, "Reverse placement failed");
    sameSet(geometry, input, restored.output);
    // Existing snapshots do not read the live graph; revision rejection belongs to session publication.
    graph.setLocalPose(tool, {{50, 60, 70}, {}});
    const auto repeated = coordinates.createPlaced(input, snapshot);
    require(repeated.status == GAGeometryStatus::Success, "Value snapshot changed after graph edit");
    sameSet(geometry, repeated.output, placed.output);
}

void cancellationAndQuaternionScale()
{
    GACgalGeometryAdapter geometry;
    GACgalCoordinateAdapter coordinates(geometry);
    const auto input = geometry.importGeometry(*cube());
    constexpr double LargeOffset = 18014398509481984.0;
    FrameTransformSnapshot path;
    path.steps = {{{{LargeOffset, 0, 0}, {}}, false}, {{{1, 0, 0}, {}}, false}, {{{-LargeOffset, 0, 0}, {}}, false}};
    const auto output = coordinates.createPlaced(input, path);
    require(output.status == GAGeometryStatus::Success, "Large-coordinate cancellation failed");
    GATransform one;
    one.values[3] = 1;
    const auto expected = geometry.createTransformed(input, one);
    require(expected.status == GAGeometryStatus::Success, "Expected translation failed");
    sameSet(geometry, output.output, expected.output);
    path.steps = {{{{}, {0.25, -0.5, 0.75, 1}}, false}};
    const auto a = coordinates.createPlaced(input, path);
    path.steps.front().localToParent.rotation = {-0.5, 1, -1.5, -2};
    const auto b = coordinates.createPlaced(input, path);
    require(a.status == GAGeometryStatus::Success && b.status == GAGeometryStatus::Success,
            "Scaled quaternion placement failed");
    sameSet(geometry, a.output, b.output);
    path.steps.push_back({path.steps.front().localToParent, true});
    const auto identity = coordinates.createPlaced(input, path);
    require(identity.status == GAGeometryStatus::Success, "Exact inverse failed");
    sameSet(geometry, identity.output, input);
}

void failuresAndEmpty()
{
    GACgalGeometryAdapter geometry;
    GACgalCoordinateAdapter coordinates(geometry);
    const auto input = geometry.importGeometry(*cube());
    const auto count = geometry.resourceCount();
    FrameTransformSnapshot invalid;
    invalid.steps = {{{{}, {0, 0, 0, 0}}, false}};
    require(coordinates.createPlaced(input, invalid).status == GAGeometryStatus::InvalidArgument,
            "Zero quaternion accepted");
    invalid.steps.front().localToParent.rotation.w = 1;
    invalid.steps.front().localToParent.translation.x = std::numeric_limits<double>::infinity();
    require(coordinates.createPlaced(input, invalid).status == GAGeometryStatus::InvalidArgument,
            "Non-finite snapshot accepted");
    require(coordinates.createPlaced(GAInvalidGeometryHandle, {}).status == GAGeometryStatus::InvalidArgument,
            "Invalid placement handle accepted");
    require(geometry.resourceCount() == count, "Failed placement published a resource");
    const auto removed = geometry.apply(GABooleanOperation::Difference, input, input);
    const auto placed = coordinates.createPlaced(removed.output, {});
    require(placed.status == GAGeometryStatus::Success && empty(geometry, placed.output), "Empty placement failed");
}
} // namespace

int main()
{
    try
    {
        explicitInputPrecision();
        exactRoundTrip();
        cancellationAndQuaternionScale();
        failuresAndEmpty();
        std::cout << "Exact coordinate snapshot placement passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
