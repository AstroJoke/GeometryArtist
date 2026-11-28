#pragma once

#include "GACoordinateFrames.h"
#include "GAGeometryTypes.h"

class GACgalGeometryAdapter;

// Optional coordinate bridge borrowing geometry, not owning a graph. Resources must outlive it.
class GACgalCoordinateAdapter
{
public:
    explicit GACgalCoordinateAdapter(GACgalGeometryAdapter& resources) noexcept;

    // Compose snapshot edges exactly in order and return a new resource.
    // Callers establish units; the domain checks graph revisions when publishing.
    [[nodiscard]] GAGeometryResult createPlaced(GAGeometryHandle source,
                                                const ga::coordinates::FrameTransformSnapshot& snapshot);

private:
    GACgalGeometryAdapter& resources_;
};
