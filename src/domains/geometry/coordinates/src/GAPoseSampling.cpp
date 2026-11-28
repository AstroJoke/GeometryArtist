#include <GAPoseSampling.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace ga::coordinates
{
namespace
{
constexpr double kSmallSine = 1e-8;
constexpr std::size_t kMaximumSamplingSegments = 1'000'000;
double dot(Quaternion a, Quaternion b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
Quaternion blend(Quaternion a, Quaternion b, double u, double v)
{
    return {u * a.x + v * b.x, u * a.y + v * b.y, u * a.z + v * b.z, u * a.w + v * b.w};
}
void validTranslation(Vector3 p)
{
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
        throw std::invalid_argument("Non-finite motion translation");
}
} // namespace
bool sameOrientation(Quaternion a, Quaternion b)
{
    a = normalized(a);
    b = normalized(b);
    return (a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w) ||
           (a.x == -b.x && a.y == -b.y && a.z == -b.z && a.w == -b.w);
}
std::vector<RigidPose> samplePoseMotion(RigidPose begin, RigidPose end, PoseSamplingPolicy policy)
{
    validTranslation(begin.translation);
    validTranslation(end.translation);
    if (!std::isfinite(policy.maximumTranslationStep) || policy.maximumTranslationStep <= 0 ||
        !std::isfinite(policy.maximumRotationStepRadians) || policy.maximumRotationStepRadians <= 0 ||
        policy.maximumRotationStepRadians > std::numbers::pi || policy.maximumSegments == 0 ||
        policy.maximumSegments > kMaximumSamplingSegments)
        throw std::invalid_argument("Invalid pose sampling policy");
    auto a = normalized(begin.rotation), b = normalized(end.rotation);
    if (dot(a, b) < 0)
        b = blend(b, b, -1, 0);
    // Relative quaternion vector + atan2 avoids loss of resolution near zero angle.
    const double rx = a.w * b.x - a.x * b.w - a.y * b.z + a.z * b.y;
    const double ry = a.w * b.y + a.x * b.z - a.y * b.w - a.z * b.x;
    const double rz = a.w * b.z - a.x * b.y + a.y * b.x - a.z * b.w;
    const double sine = std::hypot(rx, ry, rz), cosine = std::clamp(dot(a, b), 0.0, 1.0);
    const double halfAngle = sameOrientation(a, b) ? 0 : std::atan2(sine, cosine);
    const auto x = begin.translation, y = end.translation;
    const double distance = std::hypot(y.x - x.x, y.y - x.y, y.z - x.z);
    const double segments = std::max(std::ceil(distance / policy.maximumTranslationStep),
                                     std::ceil(2 * halfAngle / policy.maximumRotationStepRadians));
    if (!std::isfinite(segments) || segments > double(policy.maximumSegments))
        throw std::length_error("Pose sampling exceeds its segment budget");
    begin.rotation = a;
    end.rotation = b;
    if (segments == 0)
        return {begin};
    const auto count = static_cast<std::size_t>(segments);
    std::vector<RigidPose> result;
    result.reserve(count + 1);
    result.push_back(begin);
    for (std::size_t i = 1; i < count; ++i)
    {
        const double t = double(i) / double(count);
        const auto q = halfAngle < kSmallSine
                           ? normalized(blend(a, b, 1 - t, t))
                           : normalized(blend(a, b, std::sin((1 - t) * halfAngle) / std::sin(halfAngle),
                                              std::sin(t * halfAngle) / std::sin(halfAngle)));
        result.push_back({{std::lerp(x.x, y.x, t), std::lerp(x.y, y.y, t), std::lerp(x.z, y.z, t)}, q});
    }
    result.push_back(end);
    return result;
}
} // namespace ga::coordinates
