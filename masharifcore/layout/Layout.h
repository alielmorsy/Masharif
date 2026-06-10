#ifndef LAYOUT_H
#define LAYOUT_H
#include <memory>
#include <masharifcore/macros.h>

namespace masharif {


    struct Layout {
        // Absolute (final) position, accumulated by Node::startUpdatingPositions and
        // read by callers. computedX/Y are derived each frame from localX/Y so the
        // accumulation is idempotent (see localX/Y below).
        float computedX = 0;
        float computedY = 0;

        // Local position relative to the parent's content origin, written by the
        // layout strategies. Kept separate from computedX/Y so a clean subtree whose
        // re-solve is skipped still produces correct absolute coordinates when an
        // ancestor moves — startUpdatingPositions recomputes computedX = parentX + localX
        // from the stable local value instead of mutating it in place.
        float localX = 0;
        float localY = 0;

        float computedWidth = 0.0f;
        float computedHeight = 0.0f;
        float computedFlexBasis = 0.0f;

    };
}
#endif
