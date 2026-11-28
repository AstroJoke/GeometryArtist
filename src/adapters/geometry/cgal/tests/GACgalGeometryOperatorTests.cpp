#include "GACgalGeometryAdapter.h"
#include "GACgalGeometryTestMeshes.h"
#include "GADataStore.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <limits>

using namespace ga::cgal_test;

namespace
{
GATransform translation(double x, double y = 0, double z = 0)
{
    GATransform result;
    result.values[3] = x;
    result.values[7] = y;
    result.values[11] = z;
    return result;
}

GATransform quarterTurn()
{
    GATransform result;
    result.values[0] = 0;
    result.values[1] = -1;
    result.values[4] = 1;
    result.values[5] = 0;
    return result;
}

std::shared_ptr<const GAMeshDataObject> snapshot(GACgalGeometryAdapter& geometry, GAGeometryHandle handle)
{
    GADataStore data;
    const auto id = geometry.exportGeometry(handle, "operator result", data);
    auto mesh = std::dynamic_pointer_cast<const GAMeshDataObject>(data.access(id));
    require(mesh != nullptr, "Operator export failed");
    return mesh;
}

void expectVolume(GACgalGeometryAdapter& geometry, GAGeometryHandle handle, double expected)
{
    const auto actual = signedVolume(*snapshot(geometry, handle));
    require(std::isfinite(actual) && std::abs(actual - expected) < VolumeTolerance, "Operator volume mismatch");
}

std::array<double, 2> xBounds(GACgalGeometryAdapter& geometry, GAGeometryHandle handle)
{
    const auto data = snapshot(geometry, handle);
    const auto view = data->view();
    std::array<double, 2> bounds{std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};
    for (std::size_t i = 0; i < view.vertexCount; ++i)
    {
        bounds[0] = std::min(bounds[0], view.positions[i * CoordinateCount]);
        bounds[1] = std::max(bounds[1], view.positions[i * CoordinateCount]);
    }
    return bounds;
}

void transformsAndOwnership()
{
    GACgalGeometryAdapter geometry;
    IGATransformOperator& transformPort = geometry;
    const auto input = geometry.importGeometry(*cube());
    const auto moved = transformPort.createTransformed(input, translation(3, 4, 5));
    require(moved.status == GAGeometryStatus::Success && moved.output != input, "New transform output failed");
    require(xBounds(geometry, moved.output) == std::array<double, 2>{3, 5}, "Wrong row-major translation");
    require(xBounds(geometry, input) == std::array<double, 2>{0, 2}, "New transform mutated its input");
    require(transformPort.transform(moved.output, quarterTurn()) == GAGeometryStatus::Success,
            "Legacy in-place rotation failed");
    require(xBounds(geometry, moved.output) == std::array<double, 2>{-6, -4}, "Wrong active column-vector rotation");
    expectVolume(geometry, moved.output, 8);
    require(geometry.resourceCount() == 2, "In-place transform allocated a new handle");

    auto mirror = GATransform{};
    mirror.values[0] = -1;
    const auto mirrored = transformPort.createTransformed(input, mirror);
    require(mirrored.status == GAGeometryStatus::Success, "Reflection failed");
    expectVolume(geometry, mirrored.output, 8);
    require(xBounds(geometry, mirrored.output) == std::array<double, 2>{-2, 0}, "Reflection orientation/bounds failed");
    auto scale = GATransform{};
    scale.values[0] = 2;
    scale.values[5] = 3;
    scale.values[10] = 4;
    const auto scaled = transformPort.createTransformed(input, scale);
    require(scaled.status == GAGeometryStatus::Success, "Scale failed");
    expectVolume(geometry, scaled.output, 192);
    auto shear = GATransform{};
    shear.values[1] = 0.5;
    const auto sheared = transformPort.createTransformed(input, shear);
    require(sheared.status == GAGeometryStatus::Success, "Shear failed");
    expectVolume(geometry, sheared.output, 8);

    const auto rotated = transformPort.createTransformed(input, {});
    require(rotated.status == GAGeometryStatus::Success, "Identity copy failed");
    constexpr int RotationCount = 32;
    for (int i = 0; i < RotationCount; ++i)
    {
        require(transformPort.transform(rotated.output, quarterTurn()) == GAGeometryStatus::Success,
                "Repeated exact mesh rotation failed");
    }
    const auto remainder = geometry.apply(GABooleanOperation::Difference, rotated.output, input);
    const auto reverse = geometry.apply(GABooleanOperation::Difference, input, rotated.output);
    require(remainder.status == GAGeometryStatus::Success && reverse.status == GAGeometryStatus::Success,
            "Boolean after repeated rotation failed");
    require(snapshot(geometry, remainder.output)->empty() && snapshot(geometry, reverse.output)->empty(),
            "Repeated quarter turns changed the exact set");
    // Non-right-angle double coefficients stay exact, without claiming exact rational orthogonality.
    auto arbitrary = GATransform{};
    arbitrary.values[0] = 0.6;
    arbitrary.values[1] = -0.8;
    arbitrary.values[4] = 0.8;
    arbitrary.values[5] = 0.6;
    constexpr int ArbitraryRotationCount = 16;
    for (int i = 0; i < ArbitraryRotationCount; ++i)
    {
        require(transformPort.transform(rotated.output, arbitrary) == GAGeometryStatus::Success,
                "Repeated non-right-angle rotation failed");
    }
    expectVolume(geometry, rotated.output, 8);
    const auto detachedSnapshot = snapshot(geometry, moved.output);
    geometry.releaseGeometry(moved.output);
    require(std::abs(signedVolume(*detachedSnapshot) - 8) < VolumeTolerance,
            "Snapshot lifetime depended on native handle");
}

void failureAndPrecision()
{
    class LegacyTransform : public IGATransformOperator
    {
    public:
        GAGeometryStatus transform(GAGeometryHandle, const GATransform&) override
        {
            return GAGeometryStatus::Unsupported;
        }
    };
    LegacyTransform legacy;
    require(legacy.createTransformed(GAInvalidGeometryHandle, {}).status == GAGeometryStatus::Unsupported,
            "Legacy source-compatible transform default changed");
    GACgalGeometryAdapter geometry({1000});
    const auto input = geometry.importGeometry(*cube());
    const auto moved = geometry.createTransformed(input, translation(0.0004));
    require(moved.status == GAGeometryStatus::Success, "Sub-grid translation failed");
    require(std::abs(xBounds(geometry, moved.output)[0] - 0.0004) < VolumeTolerance,
            "Transform silently reused the import quantization policy");
    const auto before = geometry.resourceCount();
    std::array<GATransform, 4> invalid;
    invalid[0].values[0] = 0;
    invalid[1].values[12] = 0.5;
    invalid[2].values[3] = std::numeric_limits<double>::infinity();
    invalid[3].values[5] = std::numeric_limits<double>::quiet_NaN();
    for (const auto& matrix : invalid)
    {
        const auto result = geometry.createTransformed(input, matrix);
        require(result.status == GAGeometryStatus::InvalidArgument && result.output == GAInvalidGeometryHandle,
                "Invalid matrix published a resource");
        require(geometry.transform(input, matrix) == GAGeometryStatus::InvalidArgument,
                "Legacy transform accepted invalid matrix");
        require(geometry.resourceCount() == before, "Transform failure leaked");
        require(xBounds(geometry, input) == std::array<double, 2>{0, 2}, "Transform failure changed input");
    }
    require(geometry.transform(GAInvalidGeometryHandle, {}) == GAGeometryStatus::InvalidArgument,
            "Invalid transform handle accepted");
    const auto empty = geometry.apply(GABooleanOperation::Difference, input, input);
    const auto movedEmpty = geometry.createTransformed(empty.output, translation(2));
    require(movedEmpty.status == GAGeometryStatus::Success && snapshot(geometry, movedEmpty.output)->empty(),
            "Empty transform failed");
}

void cavitiesAndHulls()
{
    GACgalGeometryAdapter geometry;
    IGAConvexHullOperator& hullPort = geometry;
    const auto outer = geometry.importGeometry(*cube(0, 4));
    const auto inner = geometry.importGeometry(*cube(1, 3));
    const auto hollow = geometry.apply(GABooleanOperation::Difference, outer, inner);
    require(hollow.status == GAGeometryStatus::Success, "Cavity fixture failed");
    auto reflection = translation(5);
    reflection.values[0] = -1;
    const auto placed = geometry.createTransformed(hollow.output, reflection);
    require(placed.status == GAGeometryStatus::Success, "Cavity transform failed");
    expectVolume(geometry, placed.output, 56);
    const auto filled = hullPort.computeHull(&placed.output, 1, {});
    require(filled.status == GAGeometryStatus::Success, "Cavity hull failed");
    expectVolume(geometry, filled.output, 64);
    expectVolume(geometry, placed.output, 56);

    const auto start = geometry.importGeometry(*cube(0, 1));
    const auto end = geometry.createTransformed(start, translation(2));
    require(end.status == GAGeometryStatus::Success, "Hull placement failed");
    const GAGeometryHandle endpoints[] = {start, end.output, start};
    const auto sweep = hullPort.computeHull(endpoints, std::size(endpoints), {});
    require(sweep.status == GAGeometryStatus::Success && sweep.output != start && sweep.output != end.output,
            "Multi-input hull failed");
    expectVolume(geometry, sweep.output, 3);
    expectVolume(geometry, start, 1);
    const auto empty = geometry.apply(GABooleanOperation::Difference, start, start);
    const auto emptyHull = hullPort.computeHull(&empty.output, 1, {});
    require(emptyHull.status == GAGeometryStatus::Success && snapshot(geometry, emptyHull.output)->empty(),
            "Hull of empty resources failed");
    const auto before = geometry.resourceCount();
    require(hullPort.computeHull(nullptr, 1, {}).status == GAGeometryStatus::InvalidArgument,
            "Null hull range accepted");
    require(hullPort.computeHull(endpoints, 0, {}).status == GAGeometryStatus::InvalidArgument,
            "Zero hull count accepted");
    const GAGeometryHandle invalid[] = {start, GAInvalidGeometryHandle};
    require(hullPort.computeHull(invalid, std::size(invalid), {}).status == GAGeometryStatus::InvalidArgument,
            "Invalid hull handle accepted");
    GAGeometryOptions options;
    options.repairInput = true;
    require(hullPort.computeHull(endpoints, std::size(endpoints), options).status == GAGeometryStatus::Unsupported,
            "Hull repair option ignored");
    require(geometry.resourceCount() == before, "Hull failure published resources");
}
} // namespace

int main()
{
    try
    {
        transformsAndOwnership();
        failureAndPrecision();
        cavitiesAndHulls();
        std::cout << "Exact transforms, repeated rotations, cavity preservation and hull operations passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
