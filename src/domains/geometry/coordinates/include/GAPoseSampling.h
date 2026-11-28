#pragma once
#include <GACoordinateFrames.h>
#include <numbers>
#include <vector>
namespace ga::coordinates
{
struct PoseSamplingPolicy
{
    double maximumTranslationStep = 0.01;                      // Scene length units
    double maximumRotationStepRadians = std::numbers::pi / 36; // 5 degrees
    std::size_t maximumSegments = 4096;
};
// Linear translation plus shortest-arc SLERP, including endpoints (one pose for no motion).
// Owns no geometry. Sampling limits are not surface error bounds; full turns need waypoints.
std::vector<RigidPose> samplePoseMotion(RigidPose begin, RigidPose end, PoseSamplingPolicy policy = {});
// Sign-invariant comparison of normalized quaternion representations
bool sameOrientation(Quaternion a, Quaternion b);
} // namespace ga::coordinates
