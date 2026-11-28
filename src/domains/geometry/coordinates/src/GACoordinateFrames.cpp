#include <GACoordinateFrames.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ga::coordinates
{
namespace
{
constexpr double kHalf = 0.5;
constexpr double kTwice = 2.0;
constexpr double kMillimetresPerMetre = 1000.0;
constexpr double kMillimetresPerInch = 25.4;
void validate(Vector3 v)
{
    if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z))
        throw std::invalid_argument("coordinate must be finite");
}
Quaternion multiply(Quaternion a, Quaternion b)
{
    return normalized({a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                       a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z});
}
Vector3 rotate(Quaternion q, Vector3 v)
{
    q = normalized(q);
    const Vector3 t{kTwice * (q.y * v.z - q.z * v.y), kTwice * (q.z * v.x - q.x * v.z),
                    kTwice * (q.x * v.y - q.y * v.x)};
    Vector3 result{v.x + q.w * t.x + q.y * t.z - q.z * t.y, v.y + q.w * t.y + q.z * t.x - q.x * t.z,
                   v.z + q.w * t.z + q.x * t.y - q.y * t.x};
    validate(result);
    return result;
}
RigidPose checked(RigidPose pose)
{
    validate(pose.translation);
    pose.rotation = normalized(pose.rotation);
    return pose;
}
double millimetres(LengthUnit unit)
{
    switch (unit)
    {
    case LengthUnit::Millimetre:
        return 1;
    case LengthUnit::Metre:
        return kMillimetresPerMetre;
    case LengthUnit::Inch:
        return kMillimetresPerInch;
    default:
        throw std::invalid_argument("cannot convert an unspecified length unit");
    }
}
} // namespace
Quaternion normalized(Quaternion q)
{
    const double scale = std::max({std::abs(q.x), std::abs(q.y), std::abs(q.z), std::abs(q.w)});
    if (!std::isfinite(q.x) || !std::isfinite(q.y) || !std::isfinite(q.z) || !std::isfinite(q.w) || scale == 0)
        throw std::invalid_argument("quaternion must be finite and nonzero");
    q = {q.x / scale, q.y / scale, q.z / scale, q.w / scale};
    const double norm = std::hypot(std::hypot(q.x, q.y), std::hypot(q.z, q.w));
    return {q.x / norm, q.y / norm, q.z / norm, q.w / norm};
}
Quaternion axisAngle(Vector3 axis, double radians)
{
    validate(axis);
    if (!std::isfinite(radians))
        throw std::invalid_argument("angle must be finite");
    const auto direction = normalized({axis.x, axis.y, axis.z, 0});
    const double s = std::sin(radians * kHalf);
    return normalized({direction.x * s, direction.y * s, direction.z * s, std::cos(radians * kHalf)});
}
Vector3 transformPoint(const RigidPose& pose, Vector3 point)
{
    validate(point);
    const auto p = checked(pose);
    auto result = rotate(p.rotation, point);
    result = {result.x + p.translation.x, result.y + p.translation.y, result.z + p.translation.z};
    validate(result);
    return result;
}
RigidPose compose(const RigidPose& a, const RigidPose& b)
{
    const auto ca = checked(a), cb = checked(b);
    return {transformPoint(ca, cb.translation), multiply(ca.rotation, cb.rotation)};
}
RigidPose inverse(const RigidPose& pose)
{
    auto p = checked(pose);
    const Quaternion q{-p.rotation.x, -p.rotation.y, -p.rotation.z, p.rotation.w};
    return {rotate(q, {-p.translation.x, -p.translation.y, -p.translation.z}), q};
}
Vector3 convertLength(Vector3 value, LengthUnit from, LengthUnit to)
{
    validate(value);
    if (from == to)
        return value;
    const double scale = millimetres(from) / millimetres(to);
    Vector3 result{value.x * scale, value.y * scale, value.z * scale};
    validate(result);
    return result;
}
RigidPose FrameTransformSnapshot::approximatePose() const
{
    RigidPose result;
    for (const auto& step : steps)
    {
        auto pose = step.localToParent;
        // Approximation for display only; exact adapters independently form rational coefficients.
        const auto roundInput = [](double value, std::uint32_t denominator)
        {
            if (!denominator)
                return value;
            constexpr double MaximumExactDoubleInteger = 9'007'199'254'740'991.0;
            const double scaled = value * denominator;
            if (!std::isfinite(scaled) || std::abs(scaled) > MaximumExactDoubleInteger)
                throw std::invalid_argument("Input exceeds the supported quantization range");
            return std::round(scaled) / denominator;
        };
        auto& t = pose.translation;
        t = {roundInput(t.x, step.precision.translationDenominator),
             roundInput(t.y, step.precision.translationDenominator),
             roundInput(t.z, step.precision.translationDenominator)};
        if (step.precision.quaternionDenominator)
        {
            auto q = normalized(pose.rotation);
            pose.rotation = normalized({roundInput(q.x, step.precision.quaternionDenominator),
                                        roundInput(q.y, step.precision.quaternionDenominator),
                                        roundInput(q.z, step.precision.quaternionDenominator),
                                        roundInput(q.w, step.precision.quaternionDenominator)});
        }
        result = compose(step.inverted ? inverse(pose) : pose, result);
    }
    return result;
}
FrameGraph::FrameGraph(LengthUnit unit)
    : nodes_{{kWorldFrame, {}}}
    , unit_(unit)
{
}
FrameId FrameGraph::addFrame(FrameId parent, RigidPose pose)
{
    (void)nodes_.at(parent.value);
    pose = checked(pose);
    const FrameId id{nodes_.size()};
    nodes_.push_back({parent, pose});
    ++revision_;
    return id;
}
RigidPose FrameGraph::localPose(FrameId frame) const
{
    return nodes_.at(frame.value).pose;
}
void FrameGraph::setLocalPose(FrameId frame, RigidPose pose)
{
    if (frame == kWorldFrame)
        throw std::invalid_argument("world frame is immutable");
    auto& node = nodes_.at(frame.value);
    node.pose = checked(pose);
    ++revision_;
}
void FrameGraph::rotateLocal(FrameId frame, Quaternion delta)
{
    auto pose = localPose(frame);
    pose.rotation = multiply(pose.rotation, normalized(delta));
    setLocalPose(frame, pose);
}
void FrameGraph::rotateParentAxes(FrameId frame, Quaternion delta)
{
    auto pose = localPose(frame);
    pose.rotation = multiply(normalized(delta), pose.rotation);
    setLocalPose(frame, pose);
}
FrameTransformSnapshot FrameGraph::snapshot(FrameId from, FrameId to) const
{
    (void)nodes_.at(from.value);
    (void)nodes_.at(to.value);
    std::vector<FrameId> source, target;
    for (auto id = from; id != kWorldFrame; id = nodes_[id.value].parent)
        source.push_back(id);
    for (auto id = to; id != kWorldFrame; id = nodes_[id.value].parent)
        target.push_back(id);
    // Cancel the common ancestry before composing: no large world-coordinate subtraction.
    while (!source.empty() && !target.empty() && source.back() == target.back())
    {
        source.pop_back();
        target.pop_back();
    }
    FrameTransformSnapshot result{from, to, revision_, unit_, {}};
    for (auto id : source)
        result.steps.push_back({nodes_[id.value].pose, false});
    for (auto it = target.rbegin(); it != target.rend(); ++it)
        result.steps.push_back({nodes_[it->value].pose, true});
    return result;
}
} // namespace ga::coordinates
