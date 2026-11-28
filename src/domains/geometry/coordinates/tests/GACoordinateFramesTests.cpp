#include <GACoordinateFrames.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <source_location>
#include <stdexcept>
using namespace ga::coordinates;
void require(bool ok, std::source_location at = std::source_location::current())
{
    if (!ok)
        throw std::runtime_error("coordinate check at line " + std::to_string(at.line()));
}
void near(Vector3 a, Vector3 b)
{
    require(std::abs(a.x - b.x) < 1e-10 && std::abs(a.y - b.y) < 1e-10 && std::abs(a.z - b.z) < 1e-10);
}
template <class F> void rejects(F fn)
{
    bool rejected = false;
    try
    {
        fn();
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    require(rejected);
}
int main()
{
    try
    {
        const auto rz = axisAngle({0, 0, 1}, std::numbers::pi / 2);
        const auto rx = axisAngle({1, 0, 0}, std::numbers::pi / 2);
        FrameGraph graph(LengthUnit::Millimetre);
        const auto common = graph.addFrame(kWorldFrame, {{1e15, -1e15, 0}, rz});
        const auto stock = graph.addFrame(common, {{10, 0, 0}, rz});
        const auto path = graph.addFrame(common, {{12, 0, 0}, {}});
        const auto snapshot = graph.snapshot(path, stock);
        require(snapshot.steps.size() == 2 && snapshot.unit == LengthUnit::Millimetre);
        near(transformPoint(snapshot.approximatePose(), {1, 0, 0}), {0, -3, 0});
        const auto before = graph.revision();
        graph.setLocalPose(common, {{-100, 2, 3}, rx});
        require(graph.revision() > before);
        near(transformPoint(graph.snapshot(path, stock).approximatePose(), {1, 0, 0}), {0, -3, 0});
        graph.setLocalPose(path, {{13, 0, 0}, {}});
        near(transformPoint(snapshot.approximatePose(), {1, 0, 0}), {0, -3, 0}); // owned snapshot
        near(transformPoint(graph.snapshot(path, stock).approximatePose(), {1, 0, 0}), {0, -4, 0});
        require(graph.snapshot(stock, stock).steps.empty());
        const auto local = graph.addFrame(kWorldFrame, {{5, 6, 7}, rz});
        const auto parent = graph.addFrame(kWorldFrame, {{5, 6, 7}, rz});
        graph.rotateLocal(local, rx);
        graph.rotateParentAxes(parent, rx);
        near(transformPoint(graph.localPose(local), {0, 1, 0}), {5, 6, 8});
        near(transformPoint(graph.localPose(parent), {0, 1, 0}), {4, 6, 7});
        near(graph.localPose(local).translation, {5, 6, 7});
        const RigidPose pose{{1, 2, 3}, axisAngle({2, 3, 4}, 0.73)};
        near(transformPoint(inverse(pose), transformPoint(pose, {4, -7, 2})), {4, -7, 2});
        near(convertLength({1, 2, 3}, LengthUnit::Metre, LengthUnit::Millimetre), {1000, 2000, 3000});
        rejects(
            [&]
            {
                graph.setLocalPose(kWorldFrame, {});
            });
        rejects(
            [&]
            {
                graph.addFrame({999}, {});
            });
        rejects(
            [&]
            {
                graph.snapshot(stock, {999});
            });
        rejects(
            [&]
            {
                graph.setLocalPose(stock, {{}, {0, 0, 0, 0}});
            });
        rejects(
            [&]
            {
                graph.setLocalPose(stock, {{std::numeric_limits<double>::infinity(), 0, 0}, {}});
            });
        rejects(
            [&]
            {
                convertLength({}, LengthUnit::Unspecified, LengthUnit::Metre);
            });
        rejects(
            [&]
            {
                axisAngle({}, 1);
            });
        std::cout << "Coordinate frames passed\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
