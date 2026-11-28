#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ga::coordinates
{
inline constexpr double kQuaternionIdentityW = 1.0;
struct Vector3
{
    double x = 0, y = 0, z = 0;
};
struct Quaternion
{
    double x = 0, y = 0, z = 0, w = kQuaternionIdentityW;
};
struct RigidPose
{
    Vector3 translation;
    Quaternion rotation;
};
enum class LengthUnit
{
    Unspecified,
    Millimetre,
    Metre,
    Inch
};
struct FrameId
{
    std::size_t value = 0;
    friend bool operator==(FrameId, FrameId) = default;
};
inline constexpr FrameId kWorldFrame{};

// Right-handed, active rotations on column vectors; a pose maps local to parent.
Quaternion normalized(Quaternion q);
Quaternion axisAngle(Vector3 axis, double radians);
RigidPose compose(const RigidPose& parentFromMiddle, const RigidPose& middleFromLocal);
RigidPose inverse(const RigidPose& pose);
Vector3 transformPoint(const RigidPose& pose, Vector3 point);
Vector3 convertLength(Vector3 value, LengthUnit from, LengthUnit to);

struct TransformStep
{
    RigidPose localToParent;
    bool inverted = false;
};
struct FrameTransformSnapshot
{
    FrameId from, to;
    std::uint64_t revision = 0;
    LengthUnit unit = LengthUnit::Unspecified;
    // Edges in application order let exact adapters avoid a floating matrix round trip.
    std::vector<TransformStep> steps;
    RigidPose approximatePose() const;
};

// IDs are graph-local. Parents are immutable; no global state. Hosts synchronize writes.
class FrameGraph
{
public:
    explicit FrameGraph(LengthUnit unit = LengthUnit::Unspecified);
    FrameId addFrame(FrameId parent, RigidPose localToParent = {});
    RigidPose localPose(FrameId frame) const;
    void setLocalPose(FrameId frame, RigidPose pose);
    // Both operations keep the frame origin fixed in its parent (no orbit).
    void rotateLocal(FrameId frame, Quaternion delta);
    void rotateParentAxes(FrameId frame, Quaternion delta);
    FrameTransformSnapshot snapshot(FrameId from, FrameId to) const;
    std::uint64_t revision() const noexcept
    {
        return revision_;
    }
    LengthUnit unit() const noexcept
    {
        return unit_;
    }

private:
    struct Node
    {
        FrameId parent;
        RigidPose pose;
    };
    std::vector<Node> nodes_;
    LengthUnit unit_;
    std::uint64_t revision_ = 0;
};
} // namespace ga::coordinates
